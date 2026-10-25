// theme.js — toggle light/dark, áp qua <html data-theme> và lưu cookie.
// Light là chủ đạo (DESIGN.md). Gọi initTheme() sớm để tránh nháy màu.

import { getCookie, setCookie } from "./cookies.js";

const COOKIE_KEY = "theme";

export function applyTheme(theme) {
  const t = theme === "dark" ? "dark" : "light";
  document.documentElement.setAttribute("data-theme", t);
  return t;
}

export function getTheme() {
  return getCookie(COOKIE_KEY) === "dark" ? "dark" : "light";
}

export function toggleTheme() {
  const next = getTheme() === "dark" ? "light" : "dark";
  setCookie(COOKIE_KEY, next, 365);
  return applyTheme(next);
}

// Áp theme đã lưu ngay khi import (light mặc định nếu chưa có cookie).
export function initTheme() {
  return applyTheme(getTheme());
}

initTheme();
