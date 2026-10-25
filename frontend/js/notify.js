// notify.js — showToast(message, type) thay cho alert().
// type: "success" | "error" | "warning" | "info" (mặc định "info").

let container = null;

function ensureContainer() {
  if (container) return container;
  container = document.querySelector(".toast-container");
  if (!container) {
    container = document.createElement("div");
    container.className = "toast-container";
    document.body.appendChild(container);
  }
  return container;
}

const ICONS = {
  success: "✓",
  error: "✕",
  warning: "!",
  info: "i",
};

// confirmDialog — hộp thoại xác nhận trong app (thay cho confirm() của trình duyệt).
// Trả về Promise<boolean>: true nếu bấm nút đồng ý, false nếu Hủy / đóng.
export function confirmDialog(message, {
  title = "Xác nhận",
  okText = "Đồng ý",
  cancelText = "Hủy",
  danger = false,
} = {}) {
  return new Promise((resolve) => {
    const overlay = document.createElement("div");
    overlay.className = "confirm-overlay";

    const box = document.createElement("div");
    box.className = "confirm-box";
    box.setAttribute("role", "dialog");
    box.setAttribute("aria-modal", "true");

    const h = document.createElement("h4");
    h.className = "confirm-box__title";
    h.textContent = title;

    const p = document.createElement("p");
    p.className = "confirm-box__msg";
    p.textContent = message;

    const actions = document.createElement("div");
    actions.className = "confirm-box__actions";

    const cancelBtn = document.createElement("button");
    cancelBtn.type = "button";
    cancelBtn.className = "btn btn--ghost btn--sm";
    cancelBtn.textContent = cancelText;

    const okBtn = document.createElement("button");
    okBtn.type = "button";
    okBtn.className = "btn btn--sm " + (danger ? "btn--danger" : "btn--primary");
    okBtn.textContent = okText;

    actions.append(cancelBtn, okBtn);
    box.append(h, p, actions);
    overlay.appendChild(box);
    document.body.appendChild(overlay);

    // Hiện với hiệu ứng (CSS .is-open)
    requestAnimationFrame(() => overlay.classList.add("is-open"));
    okBtn.focus();

    const close = (result) => {
      overlay.classList.remove("is-open");
      overlay.addEventListener("transitionend", () => overlay.remove(), { once: true });
      document.removeEventListener("keydown", onKey);
      resolve(result);
    };
    const onKey = (e) => {
      if (e.key === "Escape") close(false);
      if (e.key === "Enter") close(true);
    };

    cancelBtn.addEventListener("click", () => close(false));
    okBtn.addEventListener("click", () => close(true));
    overlay.addEventListener("click", (e) => { if (e.target === overlay) close(false); });
    document.addEventListener("keydown", onKey);
  });
}

export function showToast(message, type = "info", duration = 3200) {
  const root = ensureContainer();
  const toast = document.createElement("div");
  toast.className = `toast toast--${type}`;
  toast.setAttribute("role", "alert");

  const icon = document.createElement("span");
  icon.className = "toast__icon";
  icon.textContent = ICONS[type] || ICONS.info;

  // Dùng textContent cho message để tránh XSS (message có thể đến từ API/người dùng).
  const text = document.createElement("span");
  text.textContent = message;

  toast.append(icon, text);
  root.appendChild(toast);

  setTimeout(() => {
    toast.classList.add("is-leaving");
    toast.addEventListener("transitionend", () => toast.remove(), { once: true });
  }, duration);
}
