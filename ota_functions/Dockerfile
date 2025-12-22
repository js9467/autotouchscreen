# Small, production-friendly Node image
FROM node:20-alpine

# Sharp needs libvips (alpine package)
RUN apk add --no-cache vips

WORKDIR /app

# Install deps first (better layer caching)
COPY package.json package-lock.json* ./
RUN npm ci --omit=dev || npm i --omit=dev

# Copy app
COPY server.js ./
COPY releases ./releases

# Fly expects the app to listen on 0.0.0.0 and PORT
ENV NODE_ENV=production
ENV PORT=8080
EXPOSE 8080

CMD ["node", "server.js"]
