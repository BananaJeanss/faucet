# faucet Contributing Guidelines

Hey, thanks for taking the time to contribute to faucet. Before you get started though, it's highly recommended that you take a look through the basic guidelines, as to avoid your contributions being denied.

## General Guidelines

1. **Test your changes** – Build with `make` and (at minimum) request a few files (e.g. `curl -i localhost:8080/`).
2. **Check for duplicates** – Ensure no existing issue/PR already covers your change.
3. **Use descriptive commit messages** – Follow Conventional Commits (see below).
4. **Update documentation** – Adjust code comments and the main REAMDE.md when behavior or config changes.

## Opening an Issue

Whether you have spotted a bug/problem, or just have a feature request, you should create a [new issue](https://github.com/BananaJeanss/faucet/issues) with the appropriate template.

## Creating a Pull Request

1. **Fork the repository** and create a branch (`feature/<short-name>` or `fix/<short-name>`).
2. **Build locally** via `make` (see [makefile](makefile)).
3. **Test manually** (basic file, 404 page, optional directory listing if enabled).
4. **Conventional commit messages** (examples):
   - `feat: add optional gzip content type mapping`
   - `fix: correct range header parsing edge case`
   - `docs: clarify TRUST_XREALIP warning`
   - `refactor: reduce string copies in main loop`
5. **Keep scope focused** – one logical change per PR.
6. **Avoid new runtime dependencies** unless justified (really, try to avoid it at all costs).

## Code Style (Lightweight)

- C++17; prefer standard library facilities.
- Avoid `using namespace std;` in headers (may gradually remove existing ones).
- Favor early returns for error handling (matches current code, e.g. in `main.cpp`).
- Keep functions short; consider splitting if they exceed ~150 lines (candidate: request handling block in `main.cpp`).

## Performance & Safety

- Be mindful of per-request allocations inside the main accept loop.
- Validate any new header parsing carefully (see existing patterns).
- Do not expose internal paths or system errors in HTTP responses.

## Security / Disclosure

For suspected vulnerabilities, open a private security advisory (GitHub) or contact the maintainer via the email configured in `.env` if applicable—avoid filing public issues first.

---

That's all, thanks for taking the time to read through the guidelines, happy contributing!

## License

This project is licensed under the MIT License, see the [LICENSE](/LICENSE) file for details.

### Contributors

[![contributors](https://contributors-img.web.app/image?repo=BananaJeanss/faucet)](https://github.com/BananaJeanss/faucet/graphs/contributors)
