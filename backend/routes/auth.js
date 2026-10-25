const router = require("express").Router();
const c = require("../controllers/authController");

// OAuth social login (public) — start redirect tới provider, callback nhận code.
router.get("/google", c.googleStart);
router.get("/google/callback", c.googleCallback);
router.get("/github", c.githubStart);
router.get("/github/callback", c.githubCallback);

module.exports = router;
