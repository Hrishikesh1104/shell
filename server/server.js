import dotenv from "dotenv";
dotenv.config();

import express from "express";
import http from "http";
import WebSocket, { WebSocketServer } from "ws";
import pty from "node-pty";
import path from "path";
import fs from "fs";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// security check
const ACCESS_KEY = process.env.ACCESS_KEY;
if (!ACCESS_KEY) {
  console.error("ACCESS_KEY not set. Refusing to start.");
  process.exit(1);
}

const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

// serve frontend
app.use(express.static(path.join(__dirname, "public")));

// absolute path to shell binary
const SHELL_PATH = path.join(__dirname, "..", "shell");
console.log("SHELL_PATH:", SHELL_PATH); 

wss.on("connection", (ws, req) => {
  	const url = new URL(req.url, `http://${req.headers.host}`);
	const key = url.searchParams.get("key");


  // ACCESS CONTROL
  if (key !== ACCESS_KEY) {
    ws.send("Access denied.");
    ws.close();
    return;
  }

  // per-session isolated directory
  const sessionDir = `/tmp/session-${Date.now()}-${Math.random()
    .toString(36)
    .slice(2)}`;
  fs.mkdirSync(sessionDir);

  // spawn custom shell
  const shellProcess = pty.spawn(SHELL_PATH, [], {
    cwd: sessionDir,
    env: process.env
  });

  shellProcess.on("data", data => ws.send(data));
  ws.on("message", msg => shellProcess.write(msg));

  ws.on("close", () => {
    shellProcess.kill();
    fs.rmSync(sessionDir, { recursive: true, force: true });
  });
});

server.listen(3000, () => {
  console.log("Server running on port 3000");
});
