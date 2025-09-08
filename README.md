<div align="center">
  <img src="https://github.com/BananaJeanss/faucet/blob/main/assets/fauc2.png?raw=true" alt="Faucet Banner"/>
</div>

---

# faucet

A simple and lightweight HTTP server, written in C++.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/BananaJeanss/faucet.svg?style=flat&color=yellow)](https://github.com/BananaJeanss/faucet/stargazers)
[![code size in bytes](https://img.shields.io/github/languages/code-size/BananaJeanss/faucet.svg)](https://github.com/BananaJeanss/faucet)
[![C/C++ CI](https://github.com/BananaJeanss/faucet/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/BananaJeanss/faucet/actions/workflows/c-cpp.yml)
[![Deploy to Nest](https://github.com/BananaJeanss/faucet/actions/workflows/nest.yml/badge.svg)](https://github.com/BananaJeanss/faucet/actions/workflows/nest.yml)

## Description

faucet is a simple and lightweight HTTP server written in C++ for Linux. It's intended to be basic, and features functions such as:

- Serving static files via HTTP/1.1
- Directory listing
- Configurable custom 404 page
- Default error pages for other HTTP errors /w contact info
- Logging requests to a file
- Basic HTTP authentication
- Basic ratelimiting per IP
- Toggleable X-Real-IP and X-Forwarded-For support
- Trust score system to block potential abusers
- honeypotPaths.txt for trust score system
- Customization via .env
- And more

## Quickstart (via Github Releases)

1. Download the latest release from the [releases page](https://github.com/BananaJeanss/faucet/releases).
2. Setup your folder with the following structure:

   ```tree
   /path/to/your/folder
   ├── faucet (the binary)
   ├── .env
   └── public
       ├── 404.html (custom 404 page, optional)
       ├── index.html (your homepage, optional)
       └── ... (your other static files)
   ```

3. Create a `.env` file in the same directory as the `faucet` binary. Example from .env.example:

   ```env
   # The port to run the server on
   PORT=8080

   # Document root directory, change this from 'example' to your site's directory name
   SITE_DIR=example

   # Custom 404 page, relative to site dir
   404_PAGE=404.html

   # Dir listing if no index.html found
   DIR_LISTING=true

   # Requests/second rate limit per IP, 0 for none
   REQUEST_RATELIMIT=5

   # Contact email for error pages
   CONTACT_EMAIL=webmaster@example.com

   # HTTP Authentication, uncomment to enable
   # AUTH_CREDENTIALS=user:password

   # Logging info 
   # (logs requests to .log file, if log exceeds max lines, rotates old log to .log.bak (overwrites previous .log.bak))
   TOGGLE_LOGGING=true
   LOG_MAX_LINES=5000

   # Trust X-Real-IP header from reverse proxy (e.g. nginx, caddy), may be unsafe if not behind a trusted proxy
   TRUST_XREALIP=false

   # Runs a Trust Score evaluation on each request which ranges from 0-100.
   # This is based on headers, IP range, and other factors, and cached for duration in seconds.
   # If enabled, requests below the TRUSTSCORE_THRESHOLD will be blocked with a 403 response.
   # Trust score starts at 75 aka max trust, and is lowered based on various factors.
   # Check honeypot paths checks if the request is trying to access paths such as /admin, /wp-login.php, /xmlrpc.php, etc.
   # Wouldn't recommend using honeypot paths if you are running a site that may have such paths legitimately.
   # Blockfor duration is the time in seconds to block an IP for, if it goes below the trust score threshold.
   EVALUATE_TRUSTSCORE=false
   TRUSTSCORE_THRESHOLD=10
   CHECK_HONEYPOT_PATHS=true
   BLOCKFOR_DURATION=600
   ```

> [!IMPORTANT]
> Make sure to create and fill out the .env file, otherwise a default .env file will be created.

4. Make the `faucet` binary executable:

   ```bash
   chmod +x /path/to/your/folder/faucet
   ```

5. Run the server:

   ```bash
   /path/to/your/folder/faucet
   ```

## Quickstart (building from source)

1. Clone the repo:

   ```bash
   git clone https://github.com/BananaJeanss/faucet.git
   ```

2. Navigate to the project directory:

   ```bash
   cd faucet
   ```

3. Build the project using `make`:

   ```bash
   make
   ```

4. Follow steps 2-5 from the [Quickstart (via Github Releases)](#quickstart-via-github-releases) section.

## Environment Variables

`PORT` - Port to listen on (default: 8080)

`SITE_DIR` - Document root directory (default: `public`)

`404_PAGE` - Custom 404 page, relative to site dir (default: none)

`DIR_LISTING` - Enable directory listing if no index found (default: false)

`REQUEST_RATELIMIT` - Requests/second per IP, 0 = disabled (default: 10)

`CONTACT_EMAIL` - Contact email for error pages (default: empty)

`AUTH_CREDENTIALS` - Enables Basic Auth when set to `user:password` (default: empty)

> [!WARNING]
> Basic HTTP Auth over plain HTTP is insecure (credentials are base64, not encrypted). Use only behind HTTPS/reverse proxy.

`TOGGLE_LOGGING` - Log to `server.log` (default: false in generated file; example may show true)

`LOG_MAX_LINES` - Rotate `server.log` to `server.log.bak` when exceeded (0 = no limit) (default: 5000)

`TRUST_XREALIP` - Trust `X-Real-IP` / parse first of `X-Forwarded-For` (default: false)

> [!CAUTION]
> Enabling this option may be unsafe if the server is not behind a trusted reverse proxy, as it allows clients to spoof their IP address. Use with caution.

`EVALUATE_TRUSTSCORE` - Toggle trust score system (default: false)

`TRUSTSCORE_THRESHOLD` - Block if evaluated score <= threshold (0-100, default: 10)

`CHECK_HONEYPOT_PATHS` - Enable honeypot path detection (default: false)

`BLOCKFOR_DURATION` - Seconds to block an IP after failing threshold (default: 600)

## Trust Score System

When `EVALUATE_TRUSTSCORE=true`, each request is scored (0-100, higher is better). If the (possibly lowered) score for the last minute window is <= `TRUSTSCORE_THRESHOLD`, the current request is denied with a special 403 (code 4031) and the IP is added to a temporary block list for `BLOCKFOR_DURATION` seconds.

### Factors

Negative adjustments:

- Missing User-Agent (-20)
- Unknown / unclassified User-Agent (-10)
- Suspicious tools: curl, wget, python-requests, etc. (-10)
- High request rate per minute (-5 / -10 / -20 depending on >25 / >45 / >60)
- Accessing honeypot paths (-35 per hit and cumulative penalties for multiple hits in 3 minutes)
- Many 404s per minute (-10 / -20 / -35 for >10 / >20 / >30)
- Missing Accept-Encoding (-5)
- Older/unrecognized sec-ch-ua-platform (-5)

Positive adjustments:

- Trusted/common browser tokens (+10)
- Legit platform in sec-ch-ua-platform (+5)
- Has Referer (+5)
- Lists common encodings (gzip/deflate/br/zstd) (+5)
- Private / internal IP (+15) or loopback (+35)

The lowest score observed for an IP within a rolling minute is retained (so brief spikes upward don't immediately restore trust). Score is finally clamped 0-100.

### Honeypot Paths

If `CHECK_HONEYPOT_PATHS=true`, the server loads defaults such as `/admin`, `/wp-login.php`, `/xmlrpc.php`, etc. You can provide a `honeypotPaths.txt` in the same directory as the binary to override/extend (one path per line; leading slash optional). If the file exists, defaults are replaced entirely by its contents.

### Threshold Meaning

If `TRUSTSCORE_THRESHOLD=10`, only scores >= 10 pass by default.

## honeypotPaths.txt

Optional text file, must be in same directory as the executable. One path per line, e.g.:

```text
admin
wp-login.php
test/endpoint
```

## Contributing

Contributions are welcome! Please feel free to:

- Report bugs/suggest features by [opening an issue](https://github.com/BananaJeanss/faucet/issues)
- Submit pull requests for improvements

Make sure your contribution follows the general guidelines, and if you're opening a pull request, that it isn't being worked on by someone else already.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
