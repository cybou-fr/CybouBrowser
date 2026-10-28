// Per-user install of one release. A new release is installed by hand over the previous one; there is no automatic
// update. Each release goes into its own folder (TargetDir carries the version), so a new setup never meets an
// occupied folder and the release in use stays whole until the new one is on disk. Operations run in order: the new
// files are extracted first (a damaged archive fails here), then the previous release is removed with its own
// maintenance tool, then the shortcuts point at the new release and its folder is recorded for the next setup.
// A damaged or interrupted setup therefore leaves the previous release untouched. The browser profile lives under
// %APPDATA%, outside every install folder.
function Component()
{
}

function registryKey()
{
    return "HKEY_CURRENT_USER\\Software\\" + installer.value("Name");
}

// The folder of the release installed before this one: the one recorded by the last setup, or the fixed folder used
// before releases were versioned.
function previousRelease()
{
    var tool = installer.value("MaintenanceToolName") + ".exe";
    var target = installer.value("TargetDir");
    var candidates = [installer.value(registryKey() + "\\InstallDir"),
                      installer.value("HomeDir") + "/AppData/Local/Programs/" + installer.value("Name")];
    for (var i = 0; i < candidates.length; ++i) {
        var dir = candidates[i];
        if (dir && dir !== target && installer.fileExists(dir + "/" + tool)) return dir;
    }
    return "";
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType !== "windows") return;
    var previous = previousRelease();
    if (previous !== "")
        component.addOperation("Execute", previous + "/" + installer.value("MaintenanceToolName") + ".exe", "purge", "--confirm-command");
    var exe = "@TargetDir@/bin/cybou.exe";
    if (installer.value("Name") === "CYBOU") {
        component.addOperation("CreateShortcut", exe, "@StartMenuDir@/CYBOU.lnk",
                               "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/cybou.exe", "description=CYBOU Browser");
        component.addOperation("CreateShortcut", exe, "@DesktopDir@/CYBOU.lnk",
                               "workingDirectory=@TargetDir@/bin", "iconPath=@TargetDir@/bin/cybou.exe", "description=CYBOU Browser");
    }
    component.addOperation("GlobalConfig", registryKey(), "InstallDir", "@TargetDir@");
}
