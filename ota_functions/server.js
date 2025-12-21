import express from "express";
import Busboy from "busboy";
import sharp from "sharp";
import fs from "node:fs";
import path from "node:path";

const app = express();
const releaseRoot = path.resolve(process.env.OTA_RELEASES_DIR ?? path.join(process.cwd(), "releases"));

if (!fs.existsSync(releaseRoot)) {
  fs.mkdirSync(releaseRoot, { recursive: true });
}

// Allow the ESP32 UI (served from the LAN) to call this Fly service directly
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", process.env.CORS_ORIGIN || "*");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
  if (req.method === "OPTIONS") return res.sendStatus(204);
  next();
});

app.get("/health", (req, res) => res.json({ ok: true }));

app.get("/ota/releases", (req, res) => {
  const versions = listReleaseVersions();
  const origin = getRequestOrigin(req);
  const releases = versions.map((version) => {
    const release = loadRelease(version);
    if (!release) {
      return null;
    }
    return {
      version,
      channel: release.manifest.channel || "stable",
      released_at: release.manifest.released_at || null,
      manifest: `${origin}/ota/releases/${encodeURIComponent(version)}/manifest`
    };
  }).filter(Boolean);

  res.json({ releases });
});

app.get("/ota/manifest", (req, res) => {
  const channel = typeof req.query.channel === "string" ? req.query.channel : undefined;
  const release = selectRelease(channel);
  if (!release) {
    return res.status(404).json({ error: "No OTA releases available" });
  }
  res.setHeader("Cache-Control", "no-store");
  res.json(buildManifestResponse(release.manifest, req, release.version));
});

app.get("/ota/releases/:version/manifest", (req, res) => {
  const release = loadRelease(req.params.version);
  if (!release) {
    return res.status(404).json({ error: "Release not found" });
  }
  res.setHeader("Cache-Control", "no-store");
  res.json(buildManifestResponse(release.manifest, req, release.version));
});

app.get("/ota/releases/:version/*", (req, res) => {
  const release = loadRelease(req.params.version);
  if (!release) {
    return res.status(404).json({ error: "Release not found" });
  }

  const asset = req.params[0];
  if (!asset) {
    return res.status(404).json({ error: "Asset not specified" });
  }

  const sanitized = sanitizeAssetPath(asset);
  const filePath = safeJoin(release.directory, sanitized);
  if (!filePath) {
    return res.status(404).json({ error: "Asset not found" });
  }

  let stat;
  try {
    stat = fs.statSync(filePath);
  } catch {
    return res.status(404).json({ error: "Asset not found" });
  }

  if (stat.isDirectory()) {
    return res.status(404).json({ error: "Asset not found" });
  }

  res.setHeader("Content-Length", stat.size);
  res.setHeader("Cache-Control", "public, max-age=300, immutable");
  res.setHeader("Content-Type", guessMimeType(filePath));

  const stream = fs.createReadStream(filePath);
  stream.on("error", () => {
    if (!res.headersSent) {
      res.sendStatus(500);
    } else {
      res.destroy();
    }
  });
  stream.pipe(res);
});

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

function listReleaseVersions() {
  try {
    return fs.readdirSync(releaseRoot, { withFileTypes: true })
      .filter((entry) => entry.isDirectory())
      .map((entry) => entry.name)
      .sort((a, b) => compareVersions(b, a));
  } catch {
    return [];
  }
}

function loadRelease(version) {
  if (!version) {
    return null;
  }
  const safeName = String(version).replace(/[^0-9A-Za-z._-]/g, "");
  if (!safeName) {
    return null;
  }
  const directory = safeJoin(releaseRoot, safeName);
  if (!directory || !fs.existsSync(directory)) {
    return null;
  }
  const manifestPath = safeJoin(directory, "manifest.json");
  if (!manifestPath || !fs.existsSync(manifestPath)) {
    return null;
  }
  try {
    const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
    return { version: safeName, manifest, directory };
  } catch (error) {
    console.error(`[OTA] Failed to parse manifest for ${version}:`, error);
    return null;
  }
}

function selectRelease(channel) {
  const versions = listReleaseVersions();
  for (const version of versions) {
    const release = loadRelease(version);
    if (!release) {
      continue;
    }
    const manifestChannel = release.manifest.channel || "stable";
    if (!channel || manifestChannel === channel) {
      return release;
    }
  }
  return null;
}

function buildManifestResponse(manifest, req, version) {
  const origin = getRequestOrigin(req);
  const clone = JSON.parse(JSON.stringify(manifest || {}));
  clone.version = clone.version || version;
  const firmwareNode = clone.firmware || (clone.firmware = {});
  const assetName = firmwareNode.url && !isAbsoluteUrl(firmwareNode.url)
    ? sanitizeAssetPath(firmwareNode.url)
    : "firmware.bin";

  if (!isAbsoluteUrl(firmwareNode.url)) {
    firmwareNode.url = `${origin}/ota/releases/${encodeURIComponent(version)}/${encodeAssetPath(assetName)}`;
  }

  clone.firmware = firmwareNode;
  clone.channel = clone.channel || "stable";
  clone.manifest = `${origin}/ota/releases/${encodeURIComponent(version)}/manifest`;
  return clone;
}

function sanitizeAssetPath(value) {
  const cleaned = String(value || "").replace(/\\/g, "/").replace(/^\.?\//, "");
  if (!cleaned || cleaned.includes("..")) {
    return "firmware.bin";
  }
  return cleaned;
}

function encodeAssetPath(asset) {
  return asset.split("/").map((segment) => encodeURIComponent(segment)).join("/");
}

function safeJoin(base, target) {
  const resolvedBase = path.resolve(base);
  const resolvedTarget = path.resolve(resolvedBase, target);
  if (!resolvedTarget.startsWith(resolvedBase)) {
    return null;
  }
  return resolvedTarget;
}

function guessMimeType(filePath) {
  if (filePath.endsWith(".json")) {
    return "application/json";
  }
  if (filePath.endsWith(".bin")) {
    return "application/octet-stream";
  }
  return "application/octet-stream";
}

function isAbsoluteUrl(value) {
  return /^https?:\/\//i.test(String(value || ""));
}

function compareVersions(a, b) {
  const partsA = versionParts(a);
  const partsB = versionParts(b);
  const len = Math.max(partsA.length, partsB.length);
  for (let i = 0; i < len; i++) {
    const diff = (partsA[i] || 0) - (partsB[i] || 0);
    if (diff !== 0) {
      return diff > 0 ? 1 : -1;
    }
  }
  return 0;
}

function getRequestOrigin(req) {
  const protoHeader = req.get("x-forwarded-proto");
  const proto = (protoHeader || req.protocol || "http").split(",")[0].trim() || "http";
  const host = req.get("x-forwarded-host") || req.get("host") || "localhost";
  return `${proto}://${host}`;
}

function versionParts(value) {
  return String(value || "")
    .split(/[^0-9]+/)
    .filter(Boolean)
    .map((segment) => parseInt(segment, 10));
}

app.listen(process.env.PORT || 8080, "0.0.0.0", () => {
  console.log("listening on", process.env.PORT || 8080);
});
