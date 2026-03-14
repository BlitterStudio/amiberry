# One-Time Launchpad PPA Setup

This document describes the **one-time manual steps** to set up the Launchpad PPA.
After setup, source package uploads are automated by `.github/workflows/ppa-upload.yml`.

## Prerequisites

- A Launchpad account (create at https://launchpad.net)
- Your GPG key registered with Launchpad
- Ubuntu Code of Conduct signed

## Steps

### 1. Create Launchpad Account
Go to https://launchpad.net and create an account.

### 2. Register GPG Key with Launchpad
```bash
# Export the public key in ASCII armor
gpg --armor --export D74D52525340C442A4F8B657A12B57C04E1FE282 > amiberry-signing-key.asc

# Upload to Ubuntu key server (required for Launchpad)
gpg --keyserver keyserver.ubuntu.com --send-keys D74D52525340C442A4F8B657A12B57C04E1FE282
```
Then in Launchpad → Your Profile → OpenPGP keys → "Import Key".

### 3. Create PPA
Go to https://launchpad.net/~blitterstudio/+activate-ppa
- Name: `amiberry`
- Display name: `Amiberry`
- Description: `Optimized Amiga emulator`

### 4. Sign the Ubuntu Code of Conduct
Required for PPA creation. Follow the guide at:
https://help.launchpad.net/Packaging/PPA

### 5. Add GPG Private Key to GitHub Secrets
The workflow requires `GPG_PRIVATE_KEY` as a repository secret (base64-encoded):

```bash
# Export and encode the private key
gpg --armor --export-secret-keys D74D52525340C442A4F8B657A12B57C04E1FE282 | base64 > gpg-private-key.b64
```

Add the contents of `gpg-private-key.b64` as a GitHub Actions secret named `GPG_PRIVATE_KEY`
at: https://github.com/BlitterStudio/amiberry/settings/secrets/actions

### 6. Test Upload (manual)
```bash
# In the amiberry repository:
sed -i "s/UNRELEASED/noble/" debian/changelog
dpkg-buildpackage -S -d -us -uc
debsign -k D74D52525340C442A4F8B657A12B57C04E1FE282 *.changes
dput ppa:blitterstudio/amiberry *.changes
```

After upload, Launchpad builds the package (15-30 minutes).
Monitor builds at: https://launchpad.net/~blitterstudio/+archive/ubuntu/amiberry

## PPA URL for users

Once active, users can add:
```
ppa:blitterstudio/amiberry
```
Using: `sudo add-apt-repository ppa:blitterstudio/amiberry`
