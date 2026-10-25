// dev.js — chạy song song json-server (API + static) và browser-sync (live-reload).
// Tự viết bằng child_process của Node để KHÔNG cần dependency orchestrator nào
// (tránh kéo theo shell-quote/concurrency có lỗ hổng). 0 vulnerabilities.
//
// Dùng: npm run dev   → mở http://localhost:3001 (có auto-reload).

const { spawn } = require("child_process");
const path = require("path");

function run(name, script, color) {
  // shell:true để chạy được "npm" trên Windows (tránh spawn EINVAL với npm.cmd).
  const child = spawn("npm", ["run", script], {
    cwd: path.join(__dirname, ".."),
    shell: true,
  });
  const tag = `\x1b[${color}m[${name}]\x1b[0m`;
  child.stdout.on("data", (d) => process.stdout.write(`${tag} ${d}`));
  child.stderr.on("data", (d) => process.stderr.write(`${tag} ${d}`));
  child.on("exit", (code) => {
    console.log(`${tag} thoát với mã ${code}`);
    cleanup();
  });
  return child;
}

let children = [];
function cleanup() {
  children.forEach((c) => {
    try {
      c.kill();
    } catch {}
  });
  process.exit();
}

process.on("SIGINT", cleanup);
process.on("SIGTERM", cleanup);

children = [
  run("API", "serve", "32"), // xanh lá
  run("WEB", "sync", "36"), // xanh lơ
];

console.log("Dev đang chạy — mở http://localhost:3001 (Ctrl+C để dừng)");
