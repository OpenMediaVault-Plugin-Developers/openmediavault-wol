/**
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/form/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")

Ext.define("OMV.module.admin.service.wol.Rtcwake", {
    extend : "OMV.workspace.form.Panel",
    uses   : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],

    rpcService      : "Wol",
    rpcGetMethod    : "getRtcwake",
    rpcSetMethod    : "setRtcwake",

    hideOkButton  : true,

    getFormItems    : function() {
        var me = this;
        return [{
            xtype    : "fieldset",
            title    : _("Settings"),
            defaults : {
                labelSeparator : ""
            },
            items : [{
                xtype      : "combo",
                name       : "mode",
                fieldLabel : _("Standby Mode"),
                mode       : "local",
                store      : new Ext.data.SimpleStore({
                    fields  : [ "value", "text" ],
                    data    : [
                        [ "standby", _("Standby - ACPI state S1") ],
                        [ "mem", _("Suspend-to-RAM - ACPI state S3") ],
                        [ "disk", _("Suspend-to-Disk - ACPI state S4") ],
                        [ "off", _("Poweroff - ACPI state S5") ],
                        [ "no", _("No - Set RTC wakeup time only.") ],
                        [ "on", _("On - Useful for debugging.") ]
                    ]
                }),
                displayField  : "text",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : 2
            },{
                xtype      : "compositefield",
                name       : "standbytime",
                fieldLabel : _("Standby Time"),
                width      : 200,
                items      : [{
                    xtype         : "combo",
                    name          : "standby-hour",
                    queryMode     : "local",
                    store         : Ext.Array.range(0, 23),
                    allowBlank    : false,
                    editable      : false,
                    triggerAction : "all",
                    width         : 50,
                    value         : 9,
                    reset         : function() {}
                },{
                    xtype : "displayfield",
                    value : ":"
                },{
                    xtype         : "combo",
                    name          : "standby-minute",
                    queryMode     : "local",
                    store         : Ext.Array.range(0, 59),
                    allowBlank    : false,
                    editable      : false,
                    triggerAction : "all",
                    width         : 50,
                    value         : 0,
                    reset         : function() {}
                }]
            },{
                xtype   : "button",
                name    : "standy",
                text    : _("Standby"),
                scope   : this,
                handler : Ext.Function.bind(me.onStandyButton, me, [ me ]),
                margin  : "5 0 5 0"
            }]
        }];
    },

    onStandbyButton : function() {
        var me = this;
        me.doSubmit();

        OMV.MessageBox.show({
            title   : _("Confirmation"),
            msg     : _("Are you sure you want to enter standy mode?"),
            buttons : Ext.Msg.YESNO,
            fn      : function(answer) {
                if (answer !== "yes")
                    return;

                OMV.MessageBox.wait(null, _("Entering standby mode..."));
                OMV.Rpc.request({
                    scope   : me,
                    rpcData : {
                        service : "Wol",
                        method  : "doRtcwake",
                        params  : {
                            standby_hour   : me.getForm().findField("standby_hour").getValue(),
                            standby_minute : me.getForm().findField("standby_minute").getValue(),
                            mode           : me.getForm().findField("mode").getValue()
                        }
                    },
                    success : function(id, success, response) {
                        me.doReload();
                        OMV.MessageBox.hide();
                    }
                });
            },
            scope : me,
            icon  : Ext.Msg.QUESTION
        });
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "rtcwake",
    path      : "/service/wol",
    text      : _("Rtcwake"),
    position  : 20,
    className : "OMV.module.admin.service.wol.Rtcwake"
});
