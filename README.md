<div align="center">
  <img src="assets/faucet2.png" alt="Faucet Banner"/>
</div>

---

# faucet

A simple HTTP server

## Description

faucet is a simple HTTP server written in C++ for Linux. It's intended to be basic, and features functions such as:

- Serving static files via HTTP/1.1
- Directory listing
- Configurable custom 404 page
- Default error pages for other HTTP errors /w contact info
- Logging requests to a file
- Basic HTTP authentication
- Basic ratelimiting per IP
- Customization via .env

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

3. Create a `.env` file in the same directory as the `faucet` binary. Fill out and customize as needed:

   ```env
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
    LOG_FILE=requests.log
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

`DIR_LISTING` - Enable directory listing if no index.html found (default: true)

`REQUEST_RATELIMIT` - Requests/second rate limit per IP, 0 for none (default: 10)

`CONTACT_EMAIL` - Contact email for error pages (default: none)

`AUTH_CREDENTIALS` - Basic HTTP Authentication in the format `username:password` (default: none)

> [!WARNING]
> Basic HTTP Authentication is not secure over HTTP, I wouldn't recommend using it for anything important, since there is no HTTPS support.

`TOGGLE_LOGGING` - Enable request logging to a file (default: false)

`LOG_MAX_LINES` - Maximum lines in the log file before rotating (default: 5000)

`TRUST_XREALIP` - Trust the `X-Real-IP` header from reverse proxies (default: false)

> [!CAUTION]
> Enabling this option may be unsafe if the server is not behind a trusted reverse proxy, as it allows clients to spoof their IP address. Use with caution.

## Contributing

Contributions are welcome! Please feel free to:

- Report bugs/suggest features by [opening an issue](https://github.com/BananaJeanss/faucet/issues)
- Submit pull requests for improvements

Make sure your contribution follows the general guidelines, and if you're opening a pull request, that it isn't being worked on by someone else already.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
