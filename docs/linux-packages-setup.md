# One-Time GitHub Pages + DNS Setup for packages.amiberry.com

This guide covers the **one-time manual setup** needed to activate the package repository
at `packages.amiberry.com`. The automatic publishing (on each GitHub Release) is handled
by `.github/workflows/update-repos.yml` — no manual steps needed after this setup.

---

## Step 1: Add GitHub Secrets

In GitHub → Repository Settings → Secrets and Variables → Actions → New repository secret:

| Secret Name | Value |
|-------------|-------|
| `GPG_PRIVATE_KEY` | Base64-encoded GPG private key (see `.sisyphus/evidence/task-1-gpg-key-info.txt`) |

The GPG key fingerprint is: `D74D52525340C442A4F8B657A12B57C04E1FE282`

---

## Step 2: Enable GitHub Pages via GitHub CLI

Run these commands once from your local machine (requires `gh` CLI authenticated):

```bash
# Enable GitHub Pages with Actions as the build source
gh api repos/BlitterStudio/amiberry/pages \
  --method POST \
  -f build_type=workflow \
  -f source='{"branch":"main","path":"/"}' \
  2>/dev/null || echo "Pages may already be enabled"

# Set custom domain
gh api repos/BlitterStudio/amiberry/pages \
  --method PUT \
  -f cname=packages.amiberry.com
```

Or manually in GitHub UI:
1. Go to Repository Settings → Pages
2. Source: **GitHub Actions** (NOT a branch)
3. Custom domain: `packages.amiberry.com`
4. Click Save

---

## Step 3: Add DNS Record (Namecheap)

In Namecheap DNS Management for `amiberry.com`:

| Type | Host | Value | TTL |
|------|------|-------|-----|
| CNAME | `packages` | `blitterstudio.github.io` | Automatic |

After saving, DNS propagation typically takes 1-24 hours.

---

## Step 4: Wait for HTTPS Certificate

After DNS propagates, GitHub automatically provisions a Let's Encrypt certificate.
This can take up to 24 hours.

Verify HTTPS is working:
```bash
curl -sf -o /dev/null -w "%{http_code}" https://packages.amiberry.com/
# Expected: 200 (or 404 until first workflow run)
```

**⚠️ Do NOT announce the repository URL until HTTPS is confirmed.**

---

## Step 5: Trigger First Deployment

Make a new GitHub Release (or manually trigger the workflow):
```bash
gh workflow run update-repos.yml
```

Or publish a GitHub Release — the workflow triggers automatically on `release: published`.

---

## Step 6: Verify Repository

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

| Item | Status |
|------|--------|
| GPG key in GitHub Secrets | ☐ Manual step |
| GitHub Pages enabled (Actions source) | ☐ Manual step |
| Custom domain configured | ☐ Manual step |
| CNAME DNS record added | ☐ Manual step |
| HTTPS certificate provisioned | ☐ Automatic (wait 24h) |
| First workflow run | ☐ Triggered by next Release |
