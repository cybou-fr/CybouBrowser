# Signs the given files with a self-signed code-signing certificate (CN=CYBOU Development, self-signed).
# This is NOT a trusted signature: Windows and SmartScreen still treat the publisher as unknown. It makes the files
# tamper-evident (compare the thumbprint) and exercises the signing step. The certificate is created in
# CurrentUser\My only; it is never added to any trusted root store.
#   powershell -ExecutionPolicy Bypass -File tools/package/sign_selfsigned.ps1 file1.exe file2.exe
param([Parameter(Mandatory=$true, ValueFromRemainingArguments=$true)][string[]]$Files)
$subject = 'CN=CYBOU Development (self-signed)'
$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -eq $subject -and $_.NotAfter -gt (Get-Date).AddDays(30) } | Select-Object -First 1
if (-not $cert) {
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject $subject -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddYears(2) -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256
    Write-Host "created certificate $($cert.Thumbprint)"
}
foreach ($f in $Files) {
    $r = Set-AuthenticodeSignature -FilePath $f -Certificate $cert -HashAlgorithm SHA256
    $v = Get-AuthenticodeSignature -FilePath $f
    Write-Host ("{0}: {1} signer={2} thumbprint={3}" -f $f, $v.Status, $v.SignerCertificate.Subject, $v.SignerCertificate.Thumbprint)
}
