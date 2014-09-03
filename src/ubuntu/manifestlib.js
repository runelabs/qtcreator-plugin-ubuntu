var jsonData = undefined;

function fromJSON($data) {
    try{
        jsonData = JSON.parse($data);
    }catch(err){
        //catch all exceptions
        return false;
    }

    return (jsonData !== undefined)
}

function toJSON() {
    return JSON.stringify(jsonData,null,4);
}

function getName() {
    return jsonData.name;
}

function setName($name) {
    jsonData.name = $name;
}

function setMaintainer($maintainer) {
    jsonData.maintainer = $maintainer;
}

function getMaintainer() {
    return jsonData.maintainer;
}

function setTitle($title) {
    jsonData.title = $title;
}

function getTitle() {
    return jsonData.title;
}

function setVersion($version) {
    jsonData.version = $version;
}

function getVersion() {
    return jsonData.version;
}

function setArchitecture($architecture) {
    jsonData.version = $architecture;
}

function getArchitecture() {
    return jsonData.architecture;
}

function setDescription($description) {
   jsonData.description = $description;
}

function getDescription() {
    return jsonData.description;
}

function getHooks() {
    return jsonData.hooks;
}

function setHook($hook) {
    if(!jsonData.hooks.hasOwnProperty($hook.appId))
        jsonData.hooks[$hook.appId] = {};

    if($hook.hasOwnProperty("scope")) {
        jsonData.hooks[$hook.appId]["scope"] = $hook["scope"];
        jsonData.hooks[$hook.appId]["apparmor"] = $hook["apparmor"];
    } else if ($hook.hasOwnProperty("desktop")) {
        jsonData.hooks[$hook.appId]["desktop"] = $hook["desktop"];
        jsonData.hooks[$hook.appId]["apparmor"] = $hook["apparmor"];
    }
}

function getFrameworkName() {
    return jsonData.framework;
}

function setFrameworkName($name) {
   jsonData.framework = $name;
}

function getPolicyGroups() {
/*    var appProfile = jsonData.security.profiles[$appname];
    if (appProfile===undefined) {
        jsonData.security.profiles.push( { $appname: { policy_groups: "" } } );
    }
    return appProfile.policy_groups;
*/
  return jsonData.policy_groups;

}

function setPolicyGroups($groups) {
    if ($groups.length === 0) {
        jsonData.policy_groups = [];
        return;
    }

    var $parsed_groups = $groups.split(" ");
    jsonData.policy_groups = $parsed_groups;
}

function getPolicyVersion() {
  return jsonData.policy_version.toFixed(1).toString();
}

function setPolicyVersion($string_version) {
    jsonData.policy_version = parseFloat($string_version);
}

function getAppArmorFileName($appId) {
    if(!jsonData.hooks.hasOwnProperty($appId))
        return null;
    if(!jsonData.hooks[$appId].hasOwnProperty("apparmor"))
        return null;

    return jsonData.hooks[$appId].apparmor;
}

function setAppArmorFileName($appId, $appArmorFile) {
    if(!jsonData.hooks.hasOwnProperty($appId))
        return false;

    if(!jsonData.hooks[$appId].hasOwnProperty("apparmor"))
        return false;

    jsonData.hooks[$appId].apparmor = $appArmorFile;
    return true;
}

function injectDebugPolicy () {
    if(!jsonData.hasOwnProperty("policy_groups"))
        return false;

    var debugPolicy= "debug";

    for(var i = 0; i < jsonData.policy_groups.length; i++) {
        if(jsonData.policy_groups[i] === debugPolicy )
            return true;
    }

    jsonData.policy_groups.push(debugPolicy);

    return true;
}
