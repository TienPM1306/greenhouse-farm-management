// user.js — cấu hình API tập trung + auth (login/signup/logout/phiên).
// SERVER_URL khai báo 1 chỗ; mọi fetch đi qua apiFetch() (tự gắn Bearer token).

import { setCookie, getCookie, deleteCookie } from "./cookies.js";
import { showSpinner, hideSpinner } from "./spinner.js";
import { showToast } from "./notify.js";

// ---- Cấu hình -------------------------------------------------------------
// Same-origin: backend host luôn frontend nên chỉ cần đường dẫn tương đối "/v1".
// Chạy đúng cả local (http://localhost:4000/v1) lẫn Render (https://.../v1) mà không đổi gì.
export const SERVER_URL = "/v1"; // backend Express + MongoDB (REST /v1), cùng domain

const SESSION_COOKIE = "userId";
const TOKEN_COOKIE = "authToken";

// Token JWT của phiên đăng nhập (cấp bởi POST /users/login). Dùng cho Bearer + SSE.
export function getToken() {
  return getCookie(TOKEN_COOKIE) || "";
}

// Giải mã payload JWT (phần giữa, base64url) để lấy userId — không cần verify (server đã ký).
function decodeJwtPayload(token) {
  try {
    const part = token.split(".")[1].replace(/-/g, "+").replace(/_/g, "/");
    return JSON.parse(decodeURIComponent(escape(atob(part))));
  } catch {
    return null;
  }
}

// Sau khi đăng nhập OAuth, backend redirect về ...?token=JWT. Bắt token, lưu cookie, xóa param.
// Chạy NGAY khi user.js được import (trước requireAuth ở các trang).
export function captureOAuthToken() {
  const params = new URLSearchParams(window.location.search);
  const token = params.get("token");
  if (!token) return;
  const payload = decodeJwtPayload(token);
  setCookie(TOKEN_COOKIE, token, 7);
  if (payload && payload.userId) setCookie(SESSION_COOKIE, payload.userId, 7);
  // Xóa ?token khỏi URL để không lộ trên thanh địa chỉ / lịch sử
  params.delete("token");
  const clean = window.location.pathname + (params.toString() ? "?" + params.toString() : "");
  window.history.replaceState({}, document.title, clean);
}

// Tự chạy khi import (đặt trước mọi requireAuth).
captureOAuthToken();

// ---- Wrapper fetch: tự gắn Authorization Bearer ----------------------------
export async function apiFetch(path, options = {}) {
  const token = getToken();
  const res = await fetch(`${SERVER_URL}${path}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...(options.headers || {}),
    },
  });
  if (res.status === 401) {
    // token hết hạn / thiếu → xóa phiên (trang sẽ tự về login qua requireAuth)
    deleteCookie(TOKEN_COOKIE);
    deleteCookie(SESSION_COOKIE);
  }
  if (!res.ok) throw new Error(`API ${res.status}: ${res.statusText}`);
  return res.status === 204 ? null : res.json();
}

// ---- Phiên đăng nhập -------------------------------------------------------
export function getCurrentUserId() {
  return getCookie(SESSION_COOKIE);
}

export function isLoggedIn() {
  return !!getToken() && !!getCurrentUserId();
}

// Chặn truy cập trang app khi chưa đăng nhập; gọi đầu mỗi trang nội bộ.
export function requireAuth(redirect = "login.html") {
  if (!isLoggedIn()) {
    window.location.href = redirect;
    return false;
  }
  return true;
}

// ---- Auth actions ----------------------------------------------------------
export async function login(username, password) {
  showSpinner();
  try {
    const res = await fetch(`${SERVER_URL}/users/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password }),
    });
    if (!res.ok) {
      showToast("Sai tên đăng nhập hoặc mật khẩu", "error");
      return null;
    }
    const { token, user } = await res.json();
    setCookie(TOKEN_COOKIE, token, 7);
    setCookie(SESSION_COOKIE, user.id, 7);
    showToast("Đăng nhập thành công", "success");
    return user;
  } catch (err) {
    showToast("Lỗi kết nối máy chủ", "error");
    return null;
  } finally {
    hideSpinner();
  }
}

export async function signup(payload) {
  showSpinner();
  try {
    // register là endpoint public; backend trả 409 nếu username đã tồn tại.
    const res = await fetch(`${SERVER_URL}/users/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    if (res.status === 409) {
      showToast("Tên đăng nhập đã tồn tại", "warning");
      return null;
    }
    if (!res.ok) {
      showToast("Đăng ký thất bại", "error");
      return null;
    }
    showToast("Đăng ký thành công", "success");
    return await res.json();
  } catch (err) {
    showToast("Đăng ký thất bại", "error");
    return null;
  } finally {
    hideSpinner();
  }
}

// Bước 1: gửi yêu cầu OTP — server kiểm username+email khớp rồi gửi mã 6 số qua email.
export async function sendOtp(username, email) {
  showSpinner();
  try {
    const res = await fetch(`${SERVER_URL}/users/forgot-password`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, email }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) {
      showToast(data.message || "Không gửi được mã OTP", "error");
      return false;
    }
    showToast("Đã gửi mã OTP tới email của bạn", "success");
    return true;
  } catch (err) {
    showToast("Lỗi kết nối máy chủ", "error");
    return false;
  } finally {
    hideSpinner();
  }
}

// Bước 2: xác thực mã OTP người dùng nhập.
export async function verifyOtpCode(username, code) {
  showSpinner();
  try {
    const res = await fetch(`${SERVER_URL}/users/verify-otp`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, code }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) {
      showToast(data.message || "Mã OTP không đúng", "error");
      return false;
    }
    return true;
  } catch (err) {
    showToast("Lỗi kết nối máy chủ", "error");
    return false;
  } finally {
    hideSpinner();
  }
}

// Bước 3: đặt mật khẩu mới, gửi kèm mã OTP đã xác thực.
export async function resetPassword(username, code, newPassword) {
  showSpinner();
  try {
    const res = await fetch(`${SERVER_URL}/users/reset-password`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, code, newPassword }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) {
      showToast(data.message || "Đặt lại mật khẩu thất bại", "error");
      return false;
    }
    showToast("Đặt lại mật khẩu thành công", "success");
    return true;
  } catch (err) {
    showToast("Đặt lại mật khẩu thất bại", "error");
    return false;
  } finally {
    hideSpinner();
  }
}

export async function getCurrentUser() {
  const id = getCurrentUserId();
  if (!id) return null;
  try {
    return await apiFetch(`/users/${id}`);
  } catch {
    return null;
  }
}

export function logout(redirect = "login.html") {
  deleteCookie(TOKEN_COOKIE);
  deleteCookie(SESSION_COOKIE);
  window.location.href = redirect;
}
