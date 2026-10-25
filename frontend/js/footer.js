// footer.js — chèn footer dùng chung vào phần tử .footer (nếu có).

export function renderFooter(selector = ".footer") {
  const el = document.querySelector(selector);
  if (!el) return;
  const year = new Date().getFullYear();
  el.textContent = `© ${year} Greenhouse Farm Management System — Pham Minh Tien & Tran Trung Hieu`;
}

renderFooter();
