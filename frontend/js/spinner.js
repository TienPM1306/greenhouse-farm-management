// spinner.js — showSpinner()/hideSpinner() bao quanh tác vụ gọi API.
// Tự chèn overlay vào <body> nếu chưa có (khớp .spinner-overlay trong base.css).

let overlay = null;

function ensureOverlay() {
  if (overlay) return overlay;
  overlay = document.querySelector(".spinner-overlay");
  if (!overlay) {
    overlay = document.createElement("div");
    overlay.className = "spinner-overlay";
    overlay.innerHTML = '<div class="spinner" role="status" aria-label="Đang tải"></div>';
    document.body.appendChild(overlay);
  }
  return overlay;
}

export function showSpinner() {
  ensureOverlay().classList.add("is-active");
}

export function hideSpinner() {
  ensureOverlay().classList.remove("is-active");
}
