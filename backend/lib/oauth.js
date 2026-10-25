// lib/oauth.js — OAuth Authorization Code Flow cho Google + GitHub (tự viết bằng fetch).
// client_secret đọc từ env, KHÔNG commit. Token cuối là JWT hệ thống (cấp ở authController).

const PROVIDERS = {
  google: {
    authorizeUrl: "https://accounts.google.com/o/oauth2/v2/auth",
    tokenUrl: "https://oauth2.googleapis.com/token",
    userInfoUrl: "https://www.googleapis.com/oauth2/v2/userinfo",
    scope: "openid email profile",
    clientId: () => process.env.GOOGLE_CLIENT_ID,
    clientSecret: () => process.env.GOOGLE_CLIENT_SECRET,
  },
  github: {
    authorizeUrl: "https://github.com/login/oauth/authorize",
    tokenUrl: "https://github.com/login/oauth/access_token",
    userInfoUrl: "https://api.github.com/user",
    emailsUrl: "https://api.github.com/user/emails",
    scope: "read:user user:email",
    clientId: () => process.env.GITHUB_CLIENT_ID,
    clientSecret: () => process.env.GITHUB_CLIENT_SECRET,
  },
};

// Dựng redirect_uri: ưu tiên OAUTH_BASE_URL (env), fallback theo host của request.
function getBaseUrl(req) {
  if (process.env.OAUTH_BASE_URL) return process.env.OAUTH_BASE_URL.replace(/\/$/, "");
  const proto = req.headers["x-forwarded-proto"] || req.protocol || "http";
  return `${proto}://${req.get("host")}`;
}

function getCallbackUrl(provider, req) {
  return `${getBaseUrl(req)}/v1/auth/${provider}/callback`;
}

function assertConfigured(provider) {
  const p = PROVIDERS[provider];
  if (!p) throw new Error(`Provider không hỗ trợ: ${provider}`);
  if (!p.clientId() || !p.clientSecret()) {
    throw new Error(`Thiếu cấu hình OAuth cho ${provider} (client id/secret)`);
  }
  return p;
}

// URL trang đồng ý của provider.
function buildAuthUrl(provider, state, req) {
  const p = assertConfigured(provider);
  const params = new URLSearchParams({
    client_id: p.clientId(),
    redirect_uri: getCallbackUrl(provider, req),
    scope: p.scope,
    state,
    response_type: "code",
  });
  if (provider === "google") params.set("access_type", "online");
  return `${p.authorizeUrl}?${params.toString()}`;
}

// Đổi code -> access_token.
async function exchangeCode(provider, code, req) {
  const p = assertConfigured(provider);
  const body = new URLSearchParams({
    client_id: p.clientId(),
    client_secret: p.clientSecret(),
    code,
    redirect_uri: getCallbackUrl(provider, req),
    grant_type: "authorization_code",
  });
  const res = await fetch(p.tokenUrl, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded", Accept: "application/json" },
    body,
  });
  if (!res.ok) throw new Error(`Đổi code thất bại (${provider}): ${res.status}`);
  const data = await res.json();
  if (!data.access_token) throw new Error(`Không nhận được access_token (${provider})`);
  return data.access_token;
}

// Lấy profile chuẩn hóa { email, name, avatar, providerId }.
async function fetchProfile(provider, accessToken) {
  const p = PROVIDERS[provider];
  const headers = { Authorization: `Bearer ${accessToken}`, Accept: "application/json", "User-Agent": "greenhouse-datn" };
  const res = await fetch(p.userInfoUrl, { headers });
  if (!res.ok) throw new Error(`Lấy profile thất bại (${provider}): ${res.status}`);
  const u = await res.json();

  if (provider === "google") {
    return { email: u.email, name: u.name || u.email, avatar: u.picture || "", providerId: String(u.id) };
  }
  // github: /user có thể KHÔNG có email (nếu user để private) -> gọi /user/emails lấy primary+verified
  let email = u.email;
  if (!email) {
    const er = await fetch(p.emailsUrl, { headers });
    if (er.ok) {
      const emails = await er.json();
      const primary = emails.find((e) => e.primary && e.verified) || emails.find((e) => e.verified);
      email = primary ? primary.email : null;
    }
  }
  return { email, name: u.name || u.login || email, avatar: u.avatar_url || "", providerId: String(u.id) };
}

module.exports = { PROVIDERS, getCallbackUrl, buildAuthUrl, exchangeCode, fetchProfile };
