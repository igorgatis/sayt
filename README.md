# SAYT

TODO: Explain what SAYT is.

## Installation

There are multiple ways to install SAYT:

### APE binary

[sayt.com](https://github.com/igorgatis/sayt/releases/latest/download/sayt.com)
is an [Actually Portable Executable (APE)](https://justine.lol/ape.html) binary.
Download it and place it in the root of your repository. It works on Windows,
Linux, and macOS without modification.

**Linux/macOS:**

```sh
curl -LO https://github.com/igorgatis/sayt/releases/latest/download/sayt.com
chmod +x sayt.com
```

**Windows (PowerShell):**

```powershell
Invoke-WebRequest -Uri https://github.com/igorgatis/sayt/releases/latest/download/sayt.com -OutFile sayt.com
```

### Wrapper scripts

If you'd rather not have a `sayt.com` binary checked in, use wrapper scripts:

- [saytw](https://github.com/igorgatis/sayt/releases/latest/download/saytw) for Linux/macOS
- [saytw.ps1](https://github.com/igorgatis/sayt/releases/latest/download/saytw.ps1) for Windows

**Linux/macOS:**

```sh
./saytw [args...]
```

**Windows:**

```powershell
.\saytw.ps1 [args...]
```

### Docker

Copy the binary from the container image in your Dockerfile:

```dockerfile
COPY --from=ghcr.io/igorgatis/sayt:latest /sayt /usr/local/bin/sayt
```
