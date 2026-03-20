Release Checklist
=================

Before tagging
======

1. Confirm the repo is clean on `main`.
2. Review `README.md` for any release notes or installation details that changed.
3. Confirm `config.ini` and `Configs/config_default.ini` reflect the intended defaults.
4. Verify the application name, icon, and solution metadata are still `NexPad` everywhere.

Build and package
======

1. Run the packaging script locally:

```powershell
.\scripts\package-release.ps1 -Version v0.1.0
```

2. Confirm these archives are produced under `artifacts/`:
   - `NexPad-win32-v0.1.0.zip`
   - `NexPad-x64-v0.1.0.zip`
   - `NexPad-win32-v0.1.0.zip.sha256`
   - `NexPad-x64-v0.1.0.zip.sha256`
3. Open both zip files and verify they contain:
   - `NexPad.exe`
   - `config.ini`
   - `presets/`
   - `README.md`
   - `LICENSE`
4. Verify the checksum files reference the matching zip file names and contain SHA256 hashes.
5. Smoke-test at least one packaged build on a clean directory.

GitHub workflow
======

1. Push the latest `main` branch.
2. Confirm the GitHub Actions workflow completes successfully.
3. If you are cutting a release, create and push a version tag such as `v0.1.0`.
4. Confirm the tag-triggered workflow uploads the packaged zip artifacts and checksum files.

Publish release
======

1. Draft the GitHub release notes.
2. Attach the packaged zip files and checksum files from the workflow artifacts or local `artifacts/` output.
3. Include any breaking changes, controller support changes, or config changes in the release notes.
4. Publish the release.

After release
======

1. Verify the release page renders correctly.
2. Verify the packaged files download successfully.
3. Capture any follow-up fixes for the next patch release.
