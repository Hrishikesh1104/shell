FROM ubuntu:22.04

# System build deps for node-pty
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    make \
    g++ \
    libreadline-dev \
    libncurses5-dev \
    libncursesw5-dev \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Node 20 LTS
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs

# App directory
WORKDIR /app
COPY . .

# Build your shell
RUN g++ shell.cpp -o shell -lreadline && chmod +x shell

# Backend deps — EXACTLY like your local fix
WORKDIR /app/server
RUN rm -rf node_modules package-lock.json \
 && npm cache clean --force \
 && npm install \
 && npm rebuild node-pty --build-from-source

# Expose port
EXPOSE 3000

# Start server
CMD ["node", "server.js"]
