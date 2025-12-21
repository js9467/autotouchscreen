import express from "express";
import Busboy from "busboy";
import sharp from "sharp";

const app = express();

// Allow the ESP32 UI (served from the LAN) to call this Fly service directly
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", process.env.CORS_ORIGIN || "*");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
  if (req.method === "OPTIONS") return res.sendStatus(204);
  next();
});

app.get("/health", (req, res) => res.json({ ok: true }));

/**
 * POST /optimize
 * Multipart form-data:
 * - file: image
 *
 * Query params (all optional):
 * - w: width (default 800)
 * - h: height (default 480)
 * - fit: cover|contain|fill|inside|outside (default contain)
 * - fmt: png|jpeg|rgb565 (default png)
 * - q: quality 1-100 (jpeg/webp quality; default 80)
 * - bg: background for contain padding, hex like 000000 (default 000000)
 * - rotate: 1 to auto-rotate from EXIF (default 1)
 */
app.post("/optimize", async (req, res) => {
  const bb = Busboy({ headers: req.headers, limits: { files: 1, fileSize: 20 * 1024 * 1024 } });

  const w = clampInt(req.query.w, 800, 1, 4096);
  const h = clampInt(req.query.h, 480, 1, 4096);
  const fit = ["cover", "contain", "fill", "inside", "outside"].includes(req.query.fit) ? req.query.fit : "contain";
  const fmt = ["png", "jpeg", "rgb565"].includes(req.query.fmt) ? req.query.fmt : "png";
  const q = clampInt(req.query.q, 80, 1, 100);
  const bgHex = (typeof req.query.bg === "string" && /^[0-9a-fA-F]{6}$/.test(req.query.bg)) ? req.query.bg : "000000";
  const doRotate = req.query.rotate === undefined ? true : req.query.rotate === "1";
  const preserveAlpha = req.query.alpha === "1" && fmt === "png";
  const stripMode = typeof req.query.strip === "string" ? req.query.strip : "";
  const stripTolerance = clampInt(req.query.strip_tol, 24, 1, 120);

  let fileBuffer = null;

  bb.on("file", (_name, file, info) => {
    const chunks = [];
    file.on("data", (d) => chunks.push(d));
    file.on("limit", () => {
      res.status(413).json({ error: "File too large" });
      req.destroy();
    });
    file.on("end", () => {
      fileBuffer = Buffer.concat(chunks);
    });
  });

  bb.on("error", (err) => {
    res.status(400).json({ error: "Bad multipart upload", detail: String(err) });
  });

  bb.on("finish", async () => {
    try {
      if (!fileBuffer) return res.status(400).json({ error: "No file uploaded" });

      let img = sharp(fileBuffer, { failOn: "none" });
      if (doRotate) img = img.rotate(); // auto orientation from EXIF

      // Always strip metadata
      const background = preserveAlpha ? { r: 0, g: 0, b: 0, alpha: 0 } : "#" + bgHex;
      img = img.resize({
        width: w,
        height: h,
        fit,
        background
      }).toColorspace("srgb");

      if (preserveAlpha && stripMode === "white") {
        img = await removeNearWhiteBackground(img, stripTolerance);
      }

      if (!preserveAlpha || fmt !== "png") {
        img = img.removeAlpha();
      }

      if (fmt === "png") {
        res.setHeader("Content-Type", "image/png");
        res.setHeader("Cache-Control", "public, max-age=31536000, immutable");
        const out = await img.png({ compressionLevel: 9, palette: true }).toBuffer();
        return res.status(200).send(out);
      }

      if (fmt === "jpeg") {
        res.setHeader("Content-Type", "image/jpeg");
        res.setHeader("Cache-Control", "public, max-age=31536000, immutable");
        const out = await img.jpeg({ quality: q, mozjpeg: true }).toBuffer();
        return res.status(200).send(out);
      }

      // RGB565 raw output (little-endian), no header
      // Great for embedded LCD DMA buffers
      if (fmt === "rgb565") {
        const { data, info } = await img.raw().toBuffer({ resolveWithObject: true });
        // info.channels should be 3 (RGB)
        const rgb565 = Buffer.alloc(info.width * info.height * 2);
        let j = 0;

        for (let i = 0; i < data.length; i += info.channels) {
          const r = data[i];
          const g = data[i + 1];
          const b = data[i + 2];
          const v = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
          rgb565[j++] = v & 0xff;       // low byte
          rgb565[j++] = (v >> 8) & 0xff; // high byte
        }

        res.setHeader("Content-Type", "application/octet-stream");
        res.setHeader("X-Width", String(info.width));
        res.setHeader("X-Height", String(info.height));
        res.setHeader("X-Format", "RGB565LE");
        res.setHeader("Cache-Control", "no-store");
        return res.status(200).send(rgb565);
      }

      res.status(400).json({ error: "Unsupported format" });
    } catch (e) {
      res.status(500).json({ error: "Optimization failed", detail: String(e) });
    }
  });

  req.pipe(bb);
});

function clampInt(v, def, min, max) {
  const n = parseInt(String(v ?? def), 10);
  if (Number.isNaN(n)) return def;
  return Math.max(min, Math.min(max, n));
}

async function removeNearWhiteBackground(img, tolerance = 24) {
  const clampTol = Math.max(1, Math.min(120, tolerance));
  const threshold = 255 - clampTol;
  const { data, info } = await img.ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  let modified = 0;

  for (let i = 0; i < data.length; i += info.channels) {
    const r = data[i];
    const g = data[i + 1];
    const b = data[i + 2];
    const a = data[i + 3];
    if (a === 0) continue;
    const maxChannel = Math.max(r, g, b);
    const minChannel = Math.min(r, g, b);
    if (maxChannel >= threshold && (maxChannel - minChannel) <= clampTol) {
      data[i + 3] = 0;
      modified++;
    }
  }

  return sharp(data, { raw: { width: info.width, height: info.height, channels: info.channels } }).toColorspace("srgb");
}

app.listen(process.env.PORT || 8080, "0.0.0.0", () => {
  console.log("listening on", process.env.PORT || 8080);
});
