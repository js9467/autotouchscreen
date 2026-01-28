# Small, production-friendly Node image
FROM node:20-alpine

# Cache-busting arg to force rebuild when releases change
ARG CACHEBUST=1

# Sharp needs libvips (alpine package)
RUN apk add --no-cache vips

WORKDIR /app

# Install deps first (better layer caching)
COPY package.json package-lock.json* ./
RUN npm ci --omit=dev || npm i --omit=dev

# Copy app
COPY server.js ./
# Bust cache when releases change
COPY .cachebust ./
COPY releases ./releases
COPY manifest.json ./manifest.json

# Fly expects the app to listen on 0.0.0.0 and PORT
ENV NODE_ENV=production
ENV PORT=8080
EXPOSE 8080

CMD ["node", "server.js"]
