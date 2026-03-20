NexPad vX.Y.Z
=================

Short release summary.

What Changed
======

* Describe the user-visible changes shipped in this release.
* Include configuration, controller, UI, workflow, or packaging changes as needed.

Validation
======

* List the builds, smoke tests, or other verification steps completed for this release.

Included Assets
======

* `NexPad-win32-vX.Y.Z.zip`
* `NexPad-win32-vX.Y.Z.zip.sha256`
* `NexPad-x64-vX.Y.Z.zip`
* `NexPad-x64-vX.Y.Z.zip.sha256`

Checksum Verification
======

Windows PowerShell example:

```powershell
$expected = (Get-Content .\NexPad-win32-vX.Y.Z.zip.sha256).Split(' ')[0]
$actual = (Get-FileHash .\NexPad-win32-vX.Y.Z.zip -Algorithm SHA256).Hash.ToLower()
$actual -eq $expected
```

Repeat the same check for the x64 archive before distribution or use.

Notes
======

Add any compatibility notes, known limitations, or upgrade guidance.
