// header.js — dựng topbar: nút mở sidebar (mobile) + tiêu đề trang,
// theme toggle, chuông thông báo (bado số chưa đọc), chip user.
// Dùng: renderHeader({ title }) trong mỗi trang app.

import { toggleTheme, getTheme } from "./theme.js";
import { getCurrentUser, logout, apiFetch } from "./user.js";
import { icon } from "./icons.js";

let clockTimer = null;

function initials(name = "") {
  const parts = name.trim().split(/\s+/);
  return ((parts[0]?.[0] || "") + (parts.at(-1)?.[0] || "")).toUpperCase() || "U";
}

function fmtClock(date = new Date()) {
  return [date.getHours(), date.getMinutes(), date.getSeconds()]
    .map((n) => String(n).padStart(2, "0"))
    .join(":");
}

export async function renderHeader({ title = "" } = {}) {
  const bar = document.querySelector(".topbar");
  if (!bar) return;

  bar.innerHTML = "";
  if (clockTimer) {
    clearInterval(clockTimer);
    clockTimer = null;
  }

  // ----- Trái: nút sidebar + tiêu đề -----
  const left = document.createElement("div");
  left.className = "topbar__left";

  const toggle = document.createElement("button");
  toggle.className = "sidebar-toggle";
  toggle.setAttribute("aria-label", "Mở menu");
  toggle.innerHTML = icon("duo:menu");
  toggle.addEventListener("click", (e) => {
    e.stopPropagation(); // để listener "đóng khi click ngoài" bên dưới không bắt ngay cú mở này
    document.querySelector(".app")?.classList.toggle("sidebar-open");
  });

  // Mobile: đóng sidebar khi chạm ra ngoài (vùng nền mờ). Backdrop là pseudo-element
  // ::after nên không tự bắt click được — cần listener toàn trang. Bind 1 lần duy nhất.
  if (!window.__sidebarMobileCloseBound) {
    window.__sidebarMobileCloseBound = true;
    document.addEventListener("click", (e) => {
      const app = document.querySelector(".app");
      if (!app || !app.classList.contains("sidebar-open")) return;
      // Chạm bên trong sidebar hoặc nút mở menu → không đóng.
      if (e.target.closest(".sidebar") || e.target.closest(".sidebar-toggle")) return;
      app.classList.remove("sidebar-open");
    });
  }

  const h = document.createElement("h1");
  h.className = "topbar__title";
  h.textContent = title;

  left.append(toggle, h);

  // ----- Phải: theme + chuông + user -----
  const actions = document.createElement("div");
  actions.className = "topbar__actions";

  // Đồng hồ realtime: dùng cùng định dạng HH:MM:SS với nhãn thời gian biểu đồ sensor.
  const clock = document.createElement("time");
  clock.className = "topbar-clock";
  clock.title = "Thời gian hệ thống";
  clock.dateTime = new Date().toISOString();
  const clockIcon = document.createElement("span");
  clockIcon.className = "topbar-clock__icon";
  clockIcon.innerHTML = icon("clock", { size: "16px" });
  const clockText = document.createElement("span");
  clockText.className = "topbar-clock__time";
  const paintClock = () => {
    const now = new Date();
    clock.dateTime = now.toISOString();
    clockText.textContent = fmtClock(now);
  };
  paintClock();
  clockTimer = setInterval(paintClock, 1000);
  clock.append(clockIcon, clockText);

  // Theme toggle
  const themeBtn = document.createElement("button");
  themeBtn.className = "theme-toggle";
  themeBtn.setAttribute("aria-label", "Đổi giao diện sáng/tối");
  const setIcon = () => (themeBtn.innerHTML = getTheme() === "dark" ? icon("duo:sun") : icon("duo:moon"));
  setIcon();
  themeBtn.addEventListener("click", () => {
    toggleTheme();
    setIcon();
  });

  // Chuông thông báo
  const bell = document.createElement("a");
  bell.className = "bell";
  bell.href = "alerts.html";
  bell.setAttribute("aria-label", "Thông báo");
  bell.innerHTML = icon("duo:bell");
  const badge = document.createElement("span");
  badge.className = "bell__badge";
  badge.style.display = "none";
  bell.appendChild(badge);

  // User chip — bấm để mở trang Tài khoản (đồng bộ thông tin user)
  const chip = document.createElement("a");
  chip.className = "user-chip";
  chip.href = "user-info.html";
  chip.title = "Xem tài khoản";
  const avatar = document.createElement("span");
  avatar.className = "user-chip__avatar";
  const uname = document.createElement("span");
  uname.className = "user-chip__name";
  chip.append(avatar, uname);

  // Vẽ avatar + tên vào chip từ dữ liệu user; gọi lại khi user đổi hồ sơ.
  function paintChip(user) {
    avatar.innerHTML = "";
    if (user?.avatar) {
      const img = document.createElement("img");
      img.src = user.avatar;
      img.alt = "";
      img.onerror = () => { avatar.textContent = initials(user?.name); };
      avatar.appendChild(img);
    } else {
      avatar.textContent = initials(user?.name);
    }
    uname.textContent = user?.name || "Người dùng";
  }

  paintChip(await getCurrentUser());

  // Đồng bộ ngay khi lưu hồ sơ ở trang Tài khoản (không cần chuyển trang).
  window.addEventListener("user:updated", (e) => paintChip(e.detail));

  actions.append(clock, themeBtn, bell, chip);
  bar.append(left, actions);

  // Cập nhật badge số cảnh báo (không chặn render nếu lỗi)
  try {
    const notifies = await apiFetch("/notifies");
    if (notifies?.length) {
      badge.textContent = notifies.length > 99 ? "99+" : String(notifies.length);
      badge.style.display = "flex";
    }
  } catch {
    /* bỏ qua */
  }
}
