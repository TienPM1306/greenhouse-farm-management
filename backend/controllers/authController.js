// authController — OAuth social login (Google/GitHub). Redirect qua provider rồi cấp JWT hệ thống.
const crypto = require("crypto");
const { buildAuthUrl, exchangeCode, fetchProfile } = require("../lib/oauth");
const { signToken } = require("../lib/jwt");
const User = require("../models/User");

const STATE_COOKIE = "oauth_state";
// Trang frontend nhận token sau khi đăng nhập xong (same-origin nên đường dẫn tương đối).
const FRONTEND_SUCCESS = "/pages/index.html";
const FRONTEND_LOGIN = "/pages/login.html";

function escapeRegex(s) {
  return String(s).replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

// Bước "start": sinh state, set cookie, redirect tới provider.
function makeStart(provider) {
  return function start(req, res) {
    try {
      const state = crypto.randomBytes(16).toString("hex");
      res.cookie(STATE_COOKIE, state, {
        httpOnly: true,
        maxAge: 10 * 60 * 1000, // 10 phút
        sameSite: "lax",
      });
      res.redirect(buildAuthUrl(provider, state, req));
    } catch (err) {
      console.error(`OAuth start (${provider}) lỗi:`, err.message);
      res.redirect(`${FRONTEND_LOGIN}?error=oauth_config`);
    }
  };
}

// Bước "callback": verify state -> exchange -> profile -> upsert user theo email -> JWT -> redirect.
function makeCallback(provider) {
  return async function callback(req, res) {
    // User hủy ở trang provider
    if (req.query.error) {
      return res.redirect(`${FRONTEND_LOGIN}?error=oauth_cancel`);
    }
    // Verify state chống CSRF
    const stateCookie = req.cookies ? req.cookies[STATE_COOKIE] : undefined;
    if (!req.query.state || !stateCookie || req.query.state !== stateCookie) {
      return res.redirect(`${FRONTEND_LOGIN}?error=invalid_state`);
    }
    res.clearCookie(STATE_COOKIE);
    try {
      const accessToken = await exchangeCode(provider, req.query.code, req);
      const profile = await fetchProfile(provider, accessToken);
      if (!profile.email) {
        return res.redirect(`${FRONTEND_LOGIN}?error=no_email`);
      }
      // Khớp theo email (case-insensitive)
      let user = await User.findOne({ email: new RegExp(`^${escapeRegex(profile.email)}$`, "i") });
      if (!user) {
        // Tạo user mới: username từ email (đảm bảo không trùng)
        const baseUsername = profile.email.split("@")[0];
        let username = baseUsername;
        let n = 1;
        while (await User.findOne({ username })) username = `${baseUsername}${n++}`;
        user = await User.create({
          username,
          email: profile.email,
          name: profile.name,
          avatar: profile.avatar,
          provider,
          providerId: profile.providerId,
        });
      } else if (!user.providerId) {
        // Liên kết provider vào TK sẵn có (không đổi password local)
        user.providerId = profile.providerId;
        await user.save();
      }
      const token = signToken({ userId: user._id, username: user.username });
      return res.redirect(`${FRONTEND_SUCCESS}?token=${encodeURIComponent(token)}`);
    } catch (err) {
      console.error(`OAuth callback (${provider}) lỗi:`, err.message);
      return res.redirect(`${FRONTEND_LOGIN}?error=oauth_failed`);
    }
  };
}

module.exports = {
  googleStart: makeStart("google"),
  googleCallback: makeCallback("google"),
  githubStart: makeStart("github"),
  githubCallback: makeCallback("github"),
};
