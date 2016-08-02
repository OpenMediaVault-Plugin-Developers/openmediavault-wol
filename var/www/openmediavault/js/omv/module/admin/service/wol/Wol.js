/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2014-2016 OpenMediaVault Plugin Developers
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
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/form/field/SharedFolderComboBox.js")

Ext.define("OMV.module.admin.service.wol.System", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    plugins: [{
        ptype : "configobject"
    }],

    rpcService   : "Wol",
    rpcGetMethod : "getSystem",
    rpcSetMethod : "setSystem",

    getFormItems: function() {
        var me = this;
        return [{
            xtype      : "textfield",
            name       : "name",
            fieldLabel : _("Name"),
            allowBlank : false
        },{
            xtype      : "textfield",
            name       : "mac",
            fieldLabel : _("MAC Address"),
            allowBlank : true
        },{
            xtype      : "textfield",
            name       : "ip",
            fieldLabel : _("IP Address"),
            allowBlank : true
        }];
    }
});

Ext.define("OMV.module.admin.service.wol.Systems", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format"
    ],
    uses     : [
        "OMV.module.admin.service.wol.System"
    ],

    hidePagingToolbar : false,
    autoReload        : false,
    stateful          : true,
    stateId           : "a982a76d-6804-1632-a31b-8b48c0ea6dde",
    columns           : [{
        text      : _("Name"),
        sortable  : true,
        dataIndex : "name",
        stateId   : "name"
    },{
        text      : _("MAC Address"),
        sortable  : true,
        dataIndex : "mac",
        stateId   : "mac"
    },{
        text      : _("IP Address"),
        sortable  : true,
        dataIndex : "ip",
        stateId   : "ip"
    }],

    initComponent: function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "uuid",
                    fields     : [
                        { name : "uuid", type: "string" },
                        { name : "name", type: "string" },
                        { name : "mac", type: "string" },
                        { name : "ip", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Wol",
                        method  : "getSystemList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.insert(items, 3, [{
            id       : me.getId() + "-send",
            xtype    : "button",
            text     : _("Send"),
            icon     : "images/network.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onSendButton, me, [ me ]),
            scope    : me,
            disabled : true
        },{
            id       : me.getId() + "-scan",
            xtype    : "button",
            text     : _("Scan"),
            icon     : "images/search.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onScanButton, me, [ me ]),
            scope    : me
        }]);
        return items;
    },

    onSelectionChange: function(model, records) {
        var me = this;
        me.callParent(arguments);
        // Process additional buttons.
        var tbarBtnDisabled = {
            "send" : true
        };
        if(records.length == 1) {
            tbarBtnDisabled["send"] = false;
        }
        // Update the button controls.
        Ext.Object.each(tbarBtnDisabled, function(key, value) {
            this.setToolbarButtonDisabled(key, value);
        }, me);
    },

    onAddButton: function() {
        var me = this;
        Ext.create("OMV.module.admin.service.wol.System", {
            title     : _("Add system"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onEditButton: function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.service.wol.System", {
            title     : _("Edit system"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion: function(record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Wol",
                method  : "deleteSystem",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    },

    onScanButton : function() {
        var me = this;

        OMV.MessageBox.wait(null, _("Scanning network for systems to add..."));
        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Wol",
                method   : "doScan"
            },
            success : function(id, success, response) {
                me.doReload();
                OMV.MessageBox.hide();
            }
        });
    },

    onSendButton : function() {
        var me = this;
        var record = me.getSelected();

        OMV.Rpc.request({
            scope       : me,
            relayErrors : false,
            rpcData     : {
                service  : "Wol",
                method   : "doSend",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    }
});

OMV.WorkspaceManager.registerNode({
    id      : "wol",
    path    : "/service",
    text    : _("WOL"),
    icon16  : "images/network.png",
    iconSvg : "images/network.svg"
});

OMV.WorkspaceManager.registerPanel({
    id        : "systems",
    path      : "/service/wol",
    text      : _("Systems"),
    position  : 10,
    className : "OMV.module.admin.service.wol.Systems"
});
