<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
require_once("openmediavault/object.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/util.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/notify.inc");

class OMVRpcServiceWol extends OMVRpcServiceAbstract {
    /**
     * Get the RPC service name.
     */
    public function getName() {
        return "wol";
    }

    /**
     * Initialize the RPC service.
     */
    public function initialize() {
        $this->registerMethod("getSystems");
        $this->registerMethod("getSystem");
        $this->registerMethod("setSystem");
        $this->registerMethod("deleteSystem");
        $this->registerMethod("getRtcwake");
        $this->registerMethod("setRtcwake");
        $this->registerMethod("doSend");
        $this->registerMethod("doScan");
        $this->registerMethod("doRtcwake");
    }

    public function getSystems($params, $context) {
        global $xmlConfig;
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get configuration data.
        $xpath = "//services/wol/systems/system";
        $objects = $xmlConfig->getList($xpath);
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getSystem($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

          // Get the configuration object.
        $xpath = sprintf("//services/wol/systems/system[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        return $object;
    }

    public function setSystem($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "name" : { "type" : "string" },
                "mac"  : { "type" : "string", "optional" : true },
                "ip"   : { "type" : "string", "optional" : true }
            }
        }');

        // Prepare configuration data.
        $object = array(
            "uuid" => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "name" => $params['name'],
            "mac"  => $params['mac'],
            "ip"   => $params['ip']
        );

        // Set the configuration data.
        $success = FALSE;
        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append object to configuration.
            $success = $xmlConfig->set("//services/wol/systems", array("system" => $object));
        } else {
            // Update existing configuration object.
            $success = $xmlConfig->replace("//services/wol/systems/system[uuid='{$params['uuid']}']", $object);
        }
        if(FALSE === $success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.wol.systems.system", $object);

        // Return the configuration object.
        return $object;
    }

    public function deleteSystem($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("//services/wol/system[uuid='%s']", $params['uuid']);

        return $this->deleteConfigObjectByPath($xpath,
            "org.openmediavault.services.wol.system");
    }

    public function getRtcwake($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Get the configuration object.
        $xpath = sprintf("//services/wol");
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Modify result data
        $object['standbyhour']   = intval($object['standbyhour']);
        $object['standbyminute'] = intval($object['standbyminute']);

        // Remove useless fields from the result object.
        unset($object['systems']);

        return $object;
    }

    public function setRtcwake($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "mode"          : { "type" : "string" },
                "standbyhour"   : { "type" : "integer" },
                "standbyminute" : { "type" : "integer" }
            }
        }');

        // Get existing configuration object
        $xpath = "//services/wol";
        $oldObject = $xmlConfig->get($xpath);
        if (is_null($oldObject))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Prepare configuration data.
        $object = array(
            "mode"           => $params['mode'],
            "standbyhour"    => $params['standbyhour'],
            "standbyminute"  => $params['standbyminute'],
            "systems"        => isset($oldObject['systems']) ? $oldObject['systems'] : array()
        );

        // Set configuration object
        if (FALSE === $xmlConfig->replace($xpath, $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.wol", $object);
    }

    public function doSend($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type":"object",
            "properties":{
                "uuid":{'.$GLOBALS['OMV_JSONSCHEMA_UUID'].'}
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/wol/systems/system[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        $mac = $object['mac'];
        $ip = $object['ip'];

        if( strlen($mac) == 17 )
            $cmd = "/usr/sbin/etherwake -b $mac 2>&1";
        elseif( strlen($ip) > 7 )
            $cmd = "/usr/sbin/etherwake -b $ip 2>&1";
        else
            $cmd = false;

        if( $cmd === false ) {
            if(0 !== $this->exec($cmd, $output))
                throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
        }
    }

    public function doScan($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        $cmd = "sudo /usr/bin/nast -m";

        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));

        foreach ($output as $line) {
            if( substr($line,2,1) != ":" )
                continue;

            $pieces = explode(" ", str_replace("(", "", str_replace(")", "", $line)));

            if( trim($pieces[2]) == "" )
                $pieces[2] = "n/a";

            // Prepare configuration data.
            $object = array(
                "uuid" => OMVUtil::uuid(),
                "name" => trim($pieces[2]),
                "mac"  => trim($pieces[0]),
                "ip"   => trim($pieces[1])
            );

            // Set the configuration data.
            $success = FALSE;
            $success = $xmlConfig->set("//services/wol/systems", array("system" => $object));

            if(FALSE === $success)
                throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_CREATE,
            "org.openmediavault.services.wol.systems.system", $object);
    }

    public function doRtcwake($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "mode"          : { "type" : "string" },
                "standbyhour"   : { "type" : "integer" },
                "standbyminute" : { "type" : "integer" }
            }
        }');

        $hour = intval($params['standbyhour']);
        $min = intval($params['standbyminute']);
        $time = strtotime($hour.":".$min);
        $mode = $params['mode'];

        $cmd = "/usr/sbin/rtcwake -a -t $time -m $mode";

        if(0 !== $this->exec($cmd, $output))
            throw new OMVException(OMVErrorMsg::E_EXEC_FAILED, $cmd, implode("\n", $output));
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceWol());
?>
