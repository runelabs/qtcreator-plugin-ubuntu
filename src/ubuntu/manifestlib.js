var jsonData = undefined;

function fromJSON($data) {
    jsonData = JSON.parse($data);
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

function getFrameworkName() {
    return jsonData.framework;
}

function setFrameworkName($name) {
   jsonData.framework = $name;
}

function getPolicyGroups($appname) {
/*    var appProfile = jsonData.security.profiles[$appname];
    if (appProfile===undefined) {
        jsonData.security.profiles.push( { $appname: { policy_groups: "" } } );
    }
    return appProfile.policy_groups;
*/
  return jsonData.policy_groups;

}

function setPolicyGroups($appname,$groups) {
    if ($groups.length === 0) {
        jsonData.policy_groups = [];
        return;
    }

    var $parsed_groups = $groups.split(" ");
    jsonData.policy_groups = $parsed_groups;
}

function getPolicyVersion() {
  return jsonData.policy_version.toString();
}

function setPolicyVersion($string_version) {
    jsonData.policy_version = parseFloat($string_version);
}
