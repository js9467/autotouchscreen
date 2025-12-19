# Fly.io Image Optimization Integration

This project now includes automatic cloud-based image optimization using Fly.io. Users can upload images of any size through the web interface, and they will be automatically optimized before being sent to the ESP32 device.

## Overview

The integration consists of:
1. **Fly.io Service** - A Node.js server running Sharp image processing library
2. **Web Interface** - Modified to send images to Fly for optimization
3. **ESP32 Firmware** - Receives optimized images from the web UI

## Architecture

```
User Browser → Upload Image → Fly.io Optimization Service → Optimized Image → ESP32 Device
```

### Image Flow
1. User selects an image in the web interface (up to 20MB)
2. Browser sends image to Fly.io optimization endpoint
3. Fly service resizes, optimizes, and converts the image
4. Optimized image is returned to the browser
5. Browser uploads optimized image to ESP32 via existing `/api/image/upload` endpoint

## Fly.io Service Details

**Deployment URL:** https://image-optimizer-still-flower-1282.fly.dev

### Endpoints

#### `GET /health`
Health check endpoint
- Returns: `{"ok": true}`

#### `POST /optimize`
Image optimization endpoint

**Query Parameters:**
- `w` - Width in pixels (default: 800)
- `h` - Height in pixels (default: 480)
- `fit` - Resize fit mode: `cover`, `contain`, `fill`, `inside`, `outside` (default: `contain`)
- `fmt` - Output format: `png`, `jpeg`, `rgb565` (default: `png`)
- `q` - JPEG quality 1-100 (default: 80)
- `bg` - Background color for padding (hex, default: `000000`)
- `rotate` - Auto-rotate from EXIF: `1` or `0` (default: `1`)

**Request:**
- Method: `POST`
- Content-Type: `multipart/form-data`
- Body: Image file (max 20MB)

**Response:**
- Optimized image binary data
- Content-Type varies based on format
- Headers include image dimensions for RGB565 format

### Image Type Configurations

| Type | Dimensions | Format | Quality | Use Case |
|------|-----------|---------|---------|----------|
| header | 48x36px | PNG | - | Header logo with transparency |
| splash | 300x225px | JPEG | 80 | Splash screen image |
| background | 400x240px | JPEG | 80 | Background image |
| sleep | 150x113px | JPEG | 80 | Sleep mode icon |

## Files Modified

### Web Interface (`src/web_interface.h`)
- **Function:** `handleImageUpload()` - Now sends images to Fly for optimization
- **Behavior:** 
  - Accepts images up to 20MB
  - Sends to Fly with appropriate dimensions/format for each image type
  - Displays optimization progress
  - Uploads optimized image to ESP32

### User Interface Updates
- Changed guidance text from "use prepare_images.py" to "automatic optimization via Fly.io"
- Updated info box color from warning (yellow) to info (blue)
- Added note about internet connection requirement

## Deployment

### Prerequisites
- Fly CLI installed (`iwr https://fly.io/install.ps1 -useb | iex` on Windows)
- Fly.io account (free tier available)

### Deploy/Update Service

```bash
# Login to Fly
flyctl auth login

# Deploy from project root
flyctl deploy

# Check status
flyctl status

# View logs
flyctl logs
```

### Configuration Files

**`fly.toml`** - Fly app configuration
```toml
app = 'image-optimizer-still-flower-1282'
primary_region = 'iad'

[build]
  dockerfile = 'ota_functions/Dockerfile'

[env]
  PORT = "8080"

[http_service]
  internal_port = 8080
  force_https = true
  auto_stop_machines = "stop"
  auto_start_machines = true
  min_machines_running = 0
```

**`ota_functions/Dockerfile`** - Container configuration
- Base: `node:20-alpine`
- Includes Sharp with libvips support
- Production-optimized build

**`ota_functions/package.json`** - Dependencies
- `express` - Web framework
- `sharp` - Image processing
- `busboy` - Multipart form parsing

**`ota_functions/server.js`** - Service implementation
- Image optimization logic
- RGB565 raw format support for embedded displays
- Automatic EXIF orientation handling
- Metadata stripping for smaller files

## Benefits

### For Users
- ✅ **No Local Tools Required** - No need to run Python scripts
- ✅ **Any Image Format** - Upload PNG, JPEG, GIF, WebP, etc.
- ✅ **Automatic Sizing** - Images resized to optimal dimensions
- ✅ **Quality Optimization** - Balanced quality vs file size
- ✅ **Large File Support** - Upload up to 20MB images

### For Developers
- ✅ **Serverless Auto-Scaling** - Handles multiple simultaneous uploads
- ✅ **Zero Maintenance** - Fly manages infrastructure
- ✅ **Fast Processing** - Sharp is one of the fastest image processors
- ✅ **Cost Effective** - Free tier covers typical usage

## Cost Considerations

**Free Tier Includes:**
- 3 shared-cpu-1x 256mb VMs
- 160GB outbound data transfer per month

**Usage Estimate:**
- Each image optimization: ~1-2 seconds processing
- Output size: 10-50KB per image
- With auto-stop, service only runs during active use
- Typical monthly cost: **$0** (within free tier)

## Troubleshooting

### Images Not Optimizing
1. Check internet connection on client device
2. Verify Fly service is running: `flyctl status`
3. Check browser console for errors
4. View Fly logs: `flyctl logs`

### Service Not Responding
```bash
# Restart machines
flyctl machine restart

# Check health endpoint
curl https://image-optimizer-still-flower-1282.fly.dev/health
```

### Update Fly URL
If you deploy to a different Fly app name:
1. Update URL in `src/web_interface.h`:
   ```javascript
   const flyUrl = 'https://YOUR-APP-NAME.fly.dev/optimize';
   ```
2. Rebuild and upload firmware

## Local Development

Test the optimization service locally:

```bash
cd ota_functions
npm install
node server.js
```

Service runs on http://localhost:8080

Test with curl:
```bash
curl -F "file=@test.jpg" "http://localhost:8080/optimize?w=300&h=225&fmt=jpeg&q=80"
```

## Security

- Images are processed in memory, never stored
- HTTPS enforced for all connections
- No logging of image content
- Automatic machine shutdown when idle (no persistent data)

## Performance

- Processing time: 0.5-2 seconds depending on input size
- Maximum input: 20MB
- Concurrent uploads: Auto-scales as needed
- Geographic distribution: Deployed in IAD (Virginia) region

## Future Enhancements

Possible improvements:
- [ ] Add caching for frequently used images
- [ ] Support for custom compression profiles
- [ ] WebP format support for modern browsers
- [ ] Batch image upload support
- [ ] Image preview before uploading to device

## Support

For issues:
1. Check ESP32 serial output for errors
2. Review browser console for network issues
3. Verify Fly service status
4. Check this documentation

---

**Deployment Date:** December 19, 2025  
**Service:** https://image-optimizer-still-flower-1282.fly.dev  
**Status:** ✅ Active and deployed
