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

class OMVRpcServiceWol extends OMVRpcServiceAbstract
{
    const SYSTEM = 'system';

    public function getName()
    {
        return "wol";
    }

    private function getPluginName()
    {
        return strtolower( $this->getName() );
    }

    private function getXPath()
    {
        return sprintf( "//services/%s", $this->getPluginName() );
    }

    private function getXPathSystems()
    {
        return sprintf( "%s/%ss", $this->getXPath(), self::SYSTEM );
    }

    private function getXPathSystem()
    {
        return sprintf( "%s/%s", $this->getXPathSystems(), self::SYSTEM );
    }

    private function getEventMessagePath()
    {
        return sprintf( "org.openmediavault.services.%s", $this->getPluginName() );
    }

    private function getEventMessagePathSystem()
    {
        return sprintf( "%s.%ss.%s", $this->getEventMessagePath(), self::SYSTEM, self::SYSTEM );
    }

    private function getRtcwake()
    {
        return "/usr/sbin/rtcwake";
    }

    private function getNast()
    {
        return "/usr/bin/nast";
    }

    private function getEtherwake()
    {
        return "/usr/sbin/etherwake";
    }

    public function initialize()
    {
        $this->registerMethod("getSystems");
        $this->registerMethod("getSystem");
        $this->registerMethod("setSystem");
        $this->registerMethod("deleteSystem");

        $this->registerMethod("getWake");
        $this->registerMethod("setWake");

        $this->registerMethod("doSend");
        $this->registerMethod("doScan");
        $this->registerMethod("doWake");
    }

    public function getSystems($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

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
        $objects = $xmlConfig->getList( $this->getXPathSystem() );
        if (is_null($objects))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPathSystem()
            );
        }

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getSystem($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSystem(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        return $object;
    }

    public function setSystem($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

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
        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED'])
        {
            // Append object to configuration.
            $success = $xmlConfig->set($this->getXPathSystems(), array("system" => $object));
        }
        else
        {
            // Update existing configuration object.
            $xPath = sprintf("%s[uuid='%s']", $this->getXPathSystem(), $params['uuid']);
            $success = $xmlConfig->replace($xPath, $object);
        }
        if (FALSE === $success)
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
            );
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePathSystem(),
            $object);

        // Return the configuration object.
        return $object;
    }

    public function deleteSystem($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSystem(), $params['uuid']);

        return $this->deleteConfigObjectByPath(
            $xPath,
            $this->getEventMessagePathSystem()
        );
    }

    public function getWake($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Get the configuration object.
        $object = $xmlConfig->get( $this->getXPath() );
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Modify result data
        $object['standbyhour']   = intval($object['standbyhour']);
        $object['standbyminute'] = intval($object['standbyminute']);

        // Remove useless fields from the result object.
        unset($object['systems']);

        return $object;
    }

    public function setWake($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

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
        $oldObject = $xmlConfig->get( $this->getXPath() );
        if (is_null($oldObject))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Prepare configuration data.
        $object = array(
            "mode"          => $params['mode'],
            "standbyhour"   => $params['standbyhour'],
            "standbyminute" => $params['standbyminute'],
            "systems"       => isset($oldObject['systems']) ? $oldObject['systems'] : array()
        );

        // Set configuration object
        if (FALSE === $xmlConfig->replace($this->getXPath(), $object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
            );
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );
    }

    public function doSend($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type":"object",
            "properties":{
                "uuid":{'.$GLOBALS['OMV_JSONSCHEMA_UUID'].'}
            }
        }');

        // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathSystem(), $params['uuid']);
        $object = $xmlConfig->get($xPath);
        if (is_null($object))
        {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        $mac = $object['mac'];
        $ip = $object['ip'];

        if ( strlen($mac) == 17 )
            $cmd = sprintf("export LANG=C; %s %s 2>&1", $this->getEtherwake(), $mac);
        elseif ( strlen($ip) > 7 )
            $cmd = sprintf("export LANG=C; %s %s 2>&1", $this->getEtherwake(), $ip);
        else
            $cmd = false;

        if ( $cmd !== false )
        {
            if (0 !== $this->exec($cmd, $output))
            {
                throw new OMVException(
                    OMVErrorMsg::E_EXEC_FAILED,
                    $cmd,
                    implode("\n", $output)
                );
            }
        }
    }

    public function doScan($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

        $cmd = sprintf("%s -m", $this->getNast());

        if (0 !== $this->exec($cmd, $output))
        {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }

        foreach ($output as $line)
        {
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
            $success = $xmlConfig->set($this->getXPathSystems(), array("system" => $object));

            if (FALSE === $success)
            {
                throw new OMVException(
                    OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED
                );
            }
        }

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_CREATE,
            $this->getEventMessagePathSystem(),
            $object
        );
    }

    public function doWake($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext(
            $context,
            array( "role" => OMV_ROLE_ADMINISTRATOR )
        );

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

        $cmd = sprintf("%s -a -t %s -m %s", $this->getRtcwake(), $time, $mode);

        if (0 !== $this->exec($cmd, $output))
        {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceWol());
