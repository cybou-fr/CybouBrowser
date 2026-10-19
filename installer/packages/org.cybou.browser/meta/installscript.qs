// Per-user install: Start menu and desktop shortcuts to bin/cybou.exe. The maintenance tool removes them again.
function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        var exe = "@TargetDir@/bin/cybou.exe";
        component.addOperation("CreateShortcut", exe, "@StartMenuDir@/CYBOU.lnk",
                               "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/cybou.exe", "description=CYBOU Browser");
        component.addOperation("CreateShortcut", exe, "@DesktopDir@/CYBOU.lnk",
                               "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/cybou.exe", "description=CYBOU Browser");
    }
}
