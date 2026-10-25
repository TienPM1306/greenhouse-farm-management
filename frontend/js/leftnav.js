// leftnav.js — dựng sidebar: brand + menu chính + cây Gateway→Node.
// Cây lấy gateways của user hiện tại + nodes thuộc từng gateway qua API.
// Dùng: renderLeftnav({ active }) — active là khóa menu trang hiện tại.

import { apiFetch, getCurrentUserId, getCurrentUser, logout } from "./user.js";
import { getCookie, setCookie } from "./cookies.js";
import { icon } from "./icons.js";
import { confirmDialog } from "./notify.js";

const COLLAPSE_COOKIE = "sidebarCollapsed";

// Áp trạng thái thu gọn (lưu cookie) cho khung .app.
function applyCollapsed(collapsed) {
  document.querySelector(".app")?.classList.toggle("sidebar-collapsed", collapsed);
  setCookie(COLLAPSE_COOKIE, collapsed ? "1" : "0", 90);
}

// tone = sắc màu hover/active riêng của từng mục (xem .nav__item[data-tone] trong layout.css)
const MENU = [
  { key: "overview", label: "Tổng quan", icon: "duo:dashboard", tone: "forest", href: "index.html" },
  { key: "projects", label: "Nhà kính", icon: "duo:sprout", tone: "leaf", href: "projects.html" },
  { key: "devices", label: "Thiết bị", icon: "duo:plug", tone: "teal", href: "devices.html" },
  { key: "alerts", label: "Cảnh báo", icon: "duo:bell", tone: "amber", href: "alerts.html" },
  { key: "user", label: "Tài khoản", icon: "duo:user", tone: "violet", href: "user-info.html" },
];

function el(tag, cls, text) {
  const n = document.createElement(tag);
  if (cls) n.className = cls;
  if (text != null) n.textContent = text;
  return n;
}

function initials(name = "") {
  const parts = name.trim().split(/\s+/);
  return ((parts[0]?.[0] || "") + (parts.at(-1)?.[0] || "")).toUpperCase() || "U";
}

export async function renderLeftnav({ active = "" } = {}) {
  const side = document.querySelector(".sidebar");
  if (!side) return;
  side.innerHTML = "";

  // ----- Brand -----
  const top = el("div", "sidebar__top");

  // Brand bấm được → về Tổng quan, có hover (xem layout.css)
  const brand = el("a", "sidebar__brand");
  brand.href = "index.html";
  brand.title = "Về Tổng quan";
  const brandMark = el("span", "sidebar__brand-dot");
  brandMark.innerHTML = icon("duo:plant", { size: "28px" }); // cây trong chậu — logo riêng, không trùng menu
  brand.append(
    brandMark,
    el("span", "sidebar__brand-text", "G.F.M.S")
  );

  top.append(brand);
  side.appendChild(top);

  // Khôi phục trạng thái thu gọn đã lưu
  const savedCollapsed = getCookie(COLLAPSE_COOKIE) === "1";
  const app = document.querySelector(".app");
  app?.classList.toggle("sidebar-collapsed", savedCollapsed);

  // Mở rộng khi nhấn vào BẤT KỲ vùng nào của sidebar lúc đang thu gọn.
  // (Chặn điều hướng của link để cú nhấp đầu chỉ mở sidebar.)
  side.addEventListener(
    "click",
    (e) => {
      if (app?.classList.contains("sidebar-collapsed")) {
        e.preventDefault();
        e.stopPropagation();
        applyCollapsed(false);
      }
    },
    true // capture: chặn trước khi link/nav nhận click
  );

  // Thu về thanh icon khi nhấn ra vùng ngoài sidebar (chỉ desktop).
  // Gắn 1 lần duy nhất cho toàn trang (tránh nhân bản khi re-render).
  if (!window.__sidebarOutsideBound) {
    window.__sidebarOutsideBound = true;
    document.addEventListener("click", (e) => {
      const a = document.querySelector(".app");
      if (!a || a.classList.contains("sidebar-collapsed")) return;
      if (window.innerWidth <= 900) return; // mobile dùng cơ chế overlay riêng
      // Bấm vào topbar (icon chuông/đổi giao diện, chip user…) KHÔNG thu sidebar.
      if (
        !e.target.closest(".sidebar") &&
        !e.target.closest(".sidebar-toggle") &&
        !e.target.closest(".topbar")
      ) {
        applyCollapsed(true);
      }
    });
  }

  // ----- Menu chính -----
  const nav = el("nav", "nav");
  MENU.forEach((m) => {
    const a = el("a", "nav__item" + (m.key === active ? " is-active" : ""));
    a.href = m.href;
    a.title = m.label; // tooltip khi thu gọn còn icon
    if (m.tone) a.dataset.tone = m.tone; // màu hover/active riêng theo mục
    const ic = el("span", "nav__icon");
    ic.innerHTML = icon(m.icon);
    a.append(ic, el("span", "nav__label", m.label));
    nav.appendChild(a);
  });
  side.appendChild(nav);

  // ----- Cây Gateway → Node -----
  const tree = el("div", "tree");
  tree.appendChild(el("div", "tree__group-label", "Khu vực"));
  side.appendChild(tree);

  side.appendChild(el("div", "sidebar__spacer"));

  // ----- Đáy sidebar: thông tin user (chỉ hiển thị) + Đăng xuất -----
  const foot = el("div", "sidebar__foot");
  const userBox = el("div", "sidebar__user");
  const uAvatar = el("span", "sidebar__user-avatar");
  const uName = el("span", "sidebar__user-name", "…");
  userBox.append(uAvatar, uName);

  const logoutBtn = el("button", "sidebar__logout");
  logoutBtn.type = "button";
  logoutBtn.title = "Đăng xuất";
  const logoutIc = el("span", "nav__icon");
  logoutIc.innerHTML = icon("duo:logout");
  logoutBtn.append(logoutIc, el("span", "nav__label", "Đăng xuất"));
  logoutBtn.addEventListener("click", async () => {
    const ok = await confirmDialog("Bạn có chắc muốn đăng xuất khỏi hệ thống?", {
      title: "Đăng xuất",
      okText: "Đăng xuất",
      cancelText: "Ở lại",
      danger: true,
    });
    if (ok) logout();
  });

  foot.append(userBox, logoutBtn);
  side.appendChild(foot);

  // Vẽ avatar + tên user ở đáy; gọi lại khi user đổi hồ sơ.
  function paintUserBox(u) {
    if (!u) return;
    uName.textContent = u.name || "Người dùng";
    uAvatar.innerHTML = "";
    if (u.avatar) {
      const img = document.createElement("img");
      img.src = u.avatar;
      img.alt = "";
      img.onerror = () => { uAvatar.textContent = initials(u.name); };
      uAvatar.appendChild(img);
    } else {
      uAvatar.textContent = initials(u.name);
    }
  }

  // Đổ thông tin user vào đáy (không chặn nếu lỗi)
  getCurrentUser().then(paintUserBox).catch(() => {});

  // Đồng bộ ngay khi lưu hồ sơ ở trang Tài khoản (không cần chuyển trang).
  window.addEventListener("user:updated", (e) => paintUserBox(e.detail));

  // Tải dữ liệu cây (không chặn phần còn lại nếu lỗi)
  try {
    const userId = getCurrentUserId();
    const gateways = await apiFetch(`/gateways?userId=${encodeURIComponent(userId)}`);
    for (const gw of gateways) {
      const gwEl = el("div", "tree__gateway");
      const gwIc = el("span", "tree__icon tree__icon--gateway");
      gwIc.innerHTML = icon("duo:router", { size: "18px" });
      gwEl.append(gwIc, el("span", null, gw.gatewayName || gw.projectName));
      gwEl.addEventListener("click", () => {
        window.location.href = `gateway-details.html?id=${gw.id}`;
      });
      tree.appendChild(gwEl);

      const nodes = await apiFetch(`/nodes?gatewayId=${encodeURIComponent(gw.id)}`);
      nodes.forEach((nd) => {
        const ndEl = el("a", "tree__node");
        ndEl.href = `node-details.html?id=${nd.id}`;
        const ndIc = el("span", "tree__icon tree__icon--node");
        ndIc.innerHTML = icon("duo:leaf", { size: "17px" });
        ndEl.append(ndIc, el("span", null, nd.name));
        tree.appendChild(ndEl);
      });
    }
  } catch {
    tree.appendChild(el("div", "tree__node", "Không tải được khu vực"));
  }
}
