# One-Time Setup for packages.amiberry.com

This guide covers the **one-time manual setup** needed to activate the package repository
at `packages.amiberry.com`. The automatic publishing (on each GitHub Release) is handled
by `.github/workflows/update-repos.yml` — no manual steps needed after this setup.

> **Note**: The package repository uses a **separate** GitHub repository
> (`BlitterStudio/amiberry-packages`) so it does not conflict with the existing
> `amiberry.com` GitHub Pages site served from this repository's `docs/` folder.

---

## Step 1: Add GitHub Secrets

In **this repository** (BlitterStudio/amiberry) → Settings → Secrets and Variables → Actions:

| Secret Name | Value |
|-------------|-------|
| `GPG_PRIVATE_KEY` | Base64-encoded GPG private key (see `.sisyphus/evidence/task-1-gpg-key-info.txt`) |
| `PACKAGES_REPO_TOKEN` | GitHub PAT with `public_repo` scope (create in Step 2) |

The GPG key fingerprint is: `D74D52525340C442A4F8B657A12B57C04E1FE282`

---

## Step 2: Create a GitHub Personal Access Token (PAT)

The workflow pushes built repo content to `BlitterStudio/amiberry-packages` and needs
a PAT to authenticate the cross-repo push.

1. Go to: https://github.com/settings/tokens/new (classic token)
2. Note: `amiberry-packages deploy`
3. Expiration: No expiration (or set a long expiry and rotate annually)
4. Scopes: check **`public_repo`** only
5. Click **Generate token** — copy the value immediately
6. Add it as the `PACKAGES_REPO_TOKEN` secret (Step 1 above)

---

## Step 3: Create the amiberry-packages Repository

```bash
# Create a new public repository
gh repo create BlitterStudio/amiberry-packages \
  --public \
  --description "Amiberry package repository — packages.amiberry.com"
```

Or create it manually at: https://github.com/organizations/BlitterStudio/repositories/new

---

## Step 4: Enable GitHub Pages on amiberry-packages

In the `amiberry-packages` repo → Settings → Pages:
- Source: **Deploy from a branch**
- Branch: `gh-pages`, folder: `/ (root)`
- Custom domain: `packages.amiberry.com`

Or via CLI (run after the first workflow deployment creates the `gh-pages` branch):
```bash
gh api repos/BlitterStudio/amiberry-packages/pages \
  --method PUT \
  -f cname=packages.amiberry.com
```

---

## Step 5: Add DNS Record (Namecheap)

In Namecheap DNS Management for `amiberry.com`:

| Type | Host | Value | TTL |
|------|------|-------|-----|
| CNAME | `packages` | `blitterstudio.github.io` | Automatic |

After saving, DNS propagation typically takes 1-24 hours.

> **Your existing `amiberry.com` DNS record is unaffected** — this only adds a new
> `packages` subdomain record.

---

## Step 6: Wait for HTTPS Certificate

After DNS propagates, GitHub automatically provisions a Let's Encrypt certificate
for `packages.amiberry.com`. This can take up to 24 hours.

Verify HTTPS is working:
```bash
curl -sf -o /dev/null -w "%{http_code}" https://packages.amiberry.com/
# Expected: 200 (or 404 until first workflow run)
```

**⚠️ Do NOT announce the repository URL until HTTPS is confirmed.**

---

## Step 7: Trigger First Deployment

Publish a GitHub Release — the `update-repos.yml` workflow triggers automatically
and pushes the built repo to `amiberry-packages`.

Or trigger manually:
```bash
gh workflow run update-repos.yml
```

---

## Step 8: Verify Repository

After the first deployment:
```bash
# Verify APT repo is accessible
curl -sf https://packages.amiberry.com/apt/dists/noble/InRelease | head -5

# Verify GPG key is accessible
curl -sf https://packages.amiberry.com/gpg.key | gpg --show-keys

# Test install (in Ubuntu Noble container)
docker run --rm ubuntu:24.04 bash -c "
  curl -fsSL https://packages.amiberry.com/install.sh | sh
  apt update && apt install -y amiberry
  dpkg -l amiberry
"
```

---

## Summary

| Item | How |
|------|-----|
| `GPG_PRIVATE_KEY` secret | Manual — from `.sisyphus/evidence/task-1-gpg-key-info.txt` |
| `PACKAGES_REPO_TOKEN` secret | Manual — create PAT at github.com/settings/tokens |
| Create `amiberry-packages` repo | `gh repo create BlitterStudio/amiberry-packages --public` |
| Enable Pages on `amiberry-packages` | Via repo Settings UI (after first deployment) |
| CNAME DNS record | Manual — Namecheap DNS panel (new `packages` subdomain only) |
| HTTPS certificate | Automatic — wait up to 24h after DNS |
| First deployment | Automatic — triggered by next GitHub Release |
