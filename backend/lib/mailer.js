// lib/mailer.js — gửi email OTP quên mật khẩu qua Resend HTTP API (cổng 443).
// KHÔNG dùng SMTP vì Render free chặn cổng SMTP outbound (587/465 đều timeout).
// Đọc RESEND_API_KEY + RESEND_FROM từ env (KHÔNG commit key).
const RESEND_ENDPOINT = "https://api.resend.com/emails";

function getConfig() {
  const apiKey = process.env.RESEND_API_KEY;
  if (!apiKey) {
    throw new Error("Thiếu RESEND_API_KEY trong môi trường");
  }
  // Domain test của Resend gửi ngay không cần verify; đổi bằng env nếu có domain riêng.
  const from = process.env.RESEND_FROM || "Nông trại Nhà kính <onboarding@resend.dev>";
  return { apiKey, from };
}

// Gửi mã OTP tới email người dùng. Throw nếu Resend trả lỗi (controller bắt lại).
async function sendOtpMail(email, code) {
  const { apiKey, from } = getConfig();
  const res = await fetch(RESEND_ENDPOINT, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${apiKey}`,
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      from,
      to: [email],
      subject: "Mã đặt lại mật khẩu (OTP)",
      text: `Mã OTP của bạn là: ${code}\nMã có hiệu lực trong 5 phút. Không chia sẻ mã này cho bất kỳ ai.`,
      html: `<p>Mã OTP của bạn là: <strong style="font-size:20px;letter-spacing:3px">${code}</strong></p>
<p>Mã có hiệu lực trong <strong>5 phút</strong>. Không chia sẻ mã này cho bất kỳ ai.</p>`,
    }),
  });
  if (!res.ok) {
    const detail = await res.text().catch(() => "");
    throw new Error(`Resend lỗi ${res.status}: ${detail}`);
  }
}

module.exports = { sendOtpMail };
