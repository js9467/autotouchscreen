#pragma once

const char WEB_INTERFACE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Bronco Controls v2</title>
<style>
:root{--bg:#0a0a0a;--panel:#1a1a1a;--surface:#2a2a2a;--accent:#FFA500;--text:#fff;--text-dim:#aaa;--border:#3a3a3a;--success:#1ABC9C;--danger:#E74C3C}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);line-height:1.6}
.header{background:linear-gradient(135deg,var(--panel),var(--surface));border-bottom:3px solid var(--accent);padding:20px 32px;box-shadow:0 4px 12px rgba(0,0,0,.5)}
.header h1{font-size:1.8rem;margin-bottom:4px;background:linear-gradient(90deg,var(--accent),#FFD700);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
.header .subtitle{color:var(--text-dim);font-size:.9rem}
.container{max-width:1400px;margin:0 auto;padding:24px}
.tabs{display:flex;gap:8px;background:var(--panel);padding:8px;border-radius:12px;margin-bottom:24px;overflow-x:auto;box-shadow:0 2px 8px rgba(0,0,0,.3)}
.tab-btn{padding:12px 24px;background:transparent;border:none;color:var(--text-dim);cursor:pointer;border-radius:8px;transition:all .2s;font-weight:500;white-space:nowrap}
.tab-btn:hover{background:var(--surface);color:var(--text)}
.tab-btn.active{background:var(--accent);color:var(--bg)}
.tab-content{display:none}
.tab-content.active{display:block;animation:fadeIn .3s}
@keyframes fadeIn{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}
.card{background:var(--panel);border-radius:12px;padding:24px;margin-bottom:20px;border:1px solid var(--border);box-shadow:0 4px 8px rgba(0,0,0,.2)}
.card-title{font-size:1.3rem;margin-bottom:16px;color:var(--accent);display:flex;align-items:center;gap:12px}
.card-title::before{content:'';width:4px;height:24px;background:var(--accent);border-radius:2px}
.form-group{margin-bottom:20px}
.form-label{display:block;margin-bottom:8px;color:var(--text);font-weight:500;font-size:.95rem}
.form-label small{color:var(--text-dim);font-weight:normal}
input[type=text],input[type=password],input[type=number],select,textarea{width:100%;padding:12px;background:var(--surface);border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.95rem;transition:all .2s}
input:focus,select:focus,textarea:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(255,165,0,.1)}
input[type=color]{width:100%;height:50px;padding:4px;background:var(--surface);border:1px solid var(--border);border-radius:8px;cursor:pointer}
input[type=color]::-webkit-color-swatch-wrapper{padding:0}
input[type=color]::-webkit-color-swatch{border:none;border-radius:6px}
.checkbox-wrapper{display:flex;align-items:center;gap:10px;margin:12px 0}
input[type=checkbox]{width:20px;height:20px;accent-color:var(--accent);cursor:pointer}
.btn{padding:12px 24px;border:none;border-radius:8px;font-size:.95rem;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;gap:8px}
.btn-primary{background:var(--accent);color:var(--bg)}
.btn-primary:hover{background:#FFB733;transform:translateY(-2px);box-shadow:0 4px 12px rgba(255,165,0,.3)}
.btn-secondary{background:var(--surface);color:var(--text);border:1px solid var(--border)}
.btn-secondary:hover{background:var(--border)}
.btn-success{background:var(--success);color:#fff}
.btn-danger{background:var(--danger);color:#fff}
.btn-small{padding:8px 16px;font-size:.85rem}
.btn-block{width:100%;justify-content:center}
.grid{display:grid;gap:16px}
.grid-2{grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}
.grid-3{grid-template-columns:repeat(auto-fit,minmax(250px,1fr))}
.grid-preview{background:var(--surface);border-radius:12px;padding:24px;border:2px dashed var(--border);min-height:400px;display:grid;gap:12px;position:relative;overflow:hidden}
.grid-preview-btn{background:linear-gradient(135deg,var(--accent),#FFD700);border-radius:12px;padding:16px;color:var(--bg);font-weight:600;display:flex;align-items:center;justify-content:center;text-align:center;min-height:80px;box-shadow:0 4px 8px rgba(0,0,0,.2);transition:transform .2s;flex-direction:column;gap:8px;cursor:pointer}
.grid-preview-btn:hover{transform:scale(1.02)}
.grid-preview-empty{background:var(--panel);border:2px dashed var(--border);border-radius:12px;display:flex;align-items:center;justify-content:center;color:var(--text-dim);font-size:.85rem;cursor:pointer}
.grid-preview-empty:hover{border-color:var(--accent);color:var(--accent)}
.btn-icon{font-size:1.5rem}
.modal{display:none;position:fixed;z-index:1000;left:0;top:0;width:100%;height:100%;background:rgba(0,0,0,.8);animation:fadeIn .2s;overflow-y:auto}
.modal-content{background:var(--panel);margin:5% auto;padding:32px;border-radius:12px;max-width:600px;max-height:85vh;overflow-y:auto;border:1px solid var(--border);box-shadow:0 8px 32px rgba(0,0,0,.5)}
.modal-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:24px}
.modal-title{font-size:1.5rem;color:var(--accent)}
.close-btn{background:none;border:none;color:var(--text-dim);font-size:2rem;cursor:pointer;padding:0;width:32px;height:32px;display:flex;align-items:center;justify-content:center}
.close-btn:hover{color:var(--accent)}
.wifi-list{max-height:300px;overflow-y:auto;margin-top:12px}
.wifi-item{background:var(--surface);padding:16px;margin-bottom:8px;border-radius:8px;border:1px solid var(--border);display:flex;justify-content:space-between;align-items:center;cursor:pointer;transition:all .2s}
.wifi-item:hover{background:var(--border);border-color:var(--accent)}
.wifi-item.selected{border-color:var(--accent);background:rgba(255,165,0,.1)}
.wifi-name{font-weight:600;color:var(--text)}
.wifi-signal{color:var(--text-dim);font-size:.85rem}
.wifi-strength{display:inline-block;margin-left:8px}
.can-message-item{background:var(--surface);padding:16px;margin-bottom:12px;border-radius:8px;border:1px solid var(--border)}
.can-message-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}
.can-message-name{font-weight:600;color:var(--text)}
.can-message-pgn{font-family:'Courier New',monospace;color:var(--accent);font-size:.9rem}
.can-message-data{font-family:'Courier New',monospace;font-size:.85rem;color:var(--text-dim);background:var(--panel);padding:8px;border-radius:4px;margin-top:8px;overflow-x:auto;white-space:nowrap}
.can-data-bytes{display:inline-flex;gap:6px}
.can-data-byte{background:var(--border);padding:4px 8px;border-radius:4px;font-weight:600}
.alert{padding:16px;border-radius:8px;margin-bottom:16px;display:flex;align-items:center;gap:12px;animation:slideIn .3s}
@keyframes slideIn{from{transform:translateX(-20px);opacity:0}to{transform:translateX(0);opacity:1}}
.alert-success{background:rgba(26,188,156,.1);border:1px solid var(--success);color:var(--success)}
.alert-danger{background:rgba(231,76,60,.1);border:1px solid var(--danger);color:var(--danger)}
.alert-warning{background:rgba(243,156,18,.1);border:1px solid #F39C12;color:#F39C12}
.spinner{border:3px solid var(--border);border-top:3px solid var(--accent);border-radius:50%;width:24px;height:24px;animation:spin 1s linear infinite}
@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}
.text-center{text-align:center}
.text-muted{color:var(--text-dim)}
.mb-1{margin-bottom:8px}
.mb-2{margin-bottom:16px}
.mt-2{margin-top:16px}
.flex{display:flex}
.flex-between{justify-content:space-between}
.flex-center{align-items:center}
.gap-1{gap:8px}
.gap-2{gap:16px}
</style>
</head>
<body>
<div class="header">
<h1>🐴 Bronco Controls v2</h1>
<div class="subtitle">Professional Configuration Interface</div>
</div>
<div class="container">
<div class="tabs">
<button class="tab-btn active" onclick="switchTab('wifi')">📡 WiFi</button>
<button class="tab-btn" onclick="switchTab('theme')">🎨 Theme</button>
<button class="tab-btn" onclick="switchTab('pages')">📱 Pages</button>
<button class="tab-btn" onclick="switchTab('can-library')">📚 CAN Library</button>
</div>
<div id="status-alert" style="display:none;"></div>
<div id="tab-wifi" class="tab-content active">
<div class="card">
<div class="card-title">Access Point Configuration</div>
<div class="form-group">
<label class="form-label">AP SSID</label>
<input type="text" id="ap-ssid" placeholder="Bronco-Controls"/>
</div>
<div class="form-group">
<label class="form-label">AP Password <small>(8+ characters)</small></label>
<input type="password" id="ap-password" placeholder="••••••••"/>
</div>
<div class="checkbox-wrapper">
<input type="checkbox" id="ap-enabled" checked/>
<label for="ap-enabled">Enable Access Point (Always Accessible)</label>
</div>
</div>
<div class="card">
<div class="card-title">Connect to WiFi Network</div>
<button class="btn btn-secondary btn-block mb-2" onclick="scanWiFi()">
<span id="scan-icon">🔍</span> Scan Networks
</button>
<div id="wifi-list" class="wifi-list" style="display:none;"></div>
<div class="form-group mt-2">
<label class="form-label">Network SSID</label>
<input type="text" id="sta-ssid" placeholder="Your WiFi Network"/>
</div>
<div class="form-group">
<label class="form-label">Password</label>
<input type="password" id="sta-password" placeholder="••••••••"/>
</div>
<div class="checkbox-wrapper">
<input type="checkbox" id="sta-enabled"/>
<label for="sta-enabled">Connect to this network on startup</label>
</div>
</div>
</div>
<div id="tab-theme" class="tab-content">
<div class="card">
<div class="card-title">Color Scheme</div>
<div class="grid grid-2">
<div class="form-group">
<label class="form-label">Background Color</label>
<input type="color" id="theme-bg" value="#1A1A1A"/>
</div>
<div class="form-group">
<label class="form-label">Page Background Color</label>
<input type="color" id="theme-page-bg" value="#0F0F0F"/>
</div>
<div class="form-group">
<label class="form-label">Surface/Header Color</label>
<input type="color" id="theme-surface" value="#2A2A2A"/>
</div>
<div class="form-group">
<label class="form-label">Accent Color</label>
<input type="color" id="theme-accent" value="#FFA500"/>
</div>
<div class="form-group">
<label class="form-label">Primary Text</label>
<input type="color" id="theme-text-primary" value="#FFFFFF"/>
</div>
<div class="form-group">
<label class="form-label">Secondary Text</label>
<input type="color" id="theme-text-secondary" value="#AAAAAA"/>
</div>
<div class="form-group">
<label class="form-label">Border Color</label>
<input type="color" id="theme-border" value="#3A3A3A"/>
</div>
<div class="form-group">
<label class="form-label">Header Border Color</label>
<input type="color" id="theme-header-border" value="#FFA500"/>
</div>
<div class="form-group">
<label class="form-label">Nav Button Color (Inactive)</label>
<input type="color" id="theme-nav-button" value="#3A3A3A"/>
</div>
<div class="form-group">
<label class="form-label">Nav Button Color (Active)</label>
<input type="color" id="theme-nav-active" value="#FFA500"/>
</div>
</div>
</div>
<div class="card">
<div class="card-title">Border & Styling</div>
<div class="grid grid-2">
<div class="form-group">
<label class="form-label">Button Corner Radius (px)</label>
<input type="number" id="theme-radius" value="12" min="0" max="50"/>
</div>
<div class="form-group">
<label class="form-label">Button Border Width (px)</label>
<input type="number" id="theme-border-width" value="2" min="0" max="10"/>
</div>
<div class="form-group">
<label class="form-label">Header Border Width (px)</label>
<input type="number" id="theme-header-border-width" value="0" min="0" max="10"/>
</div>
</div>
<button class="btn btn-primary mt-2" onclick="applyThemePreview()">🎨 Preview Theme</button>
</div>
</div>
<div id="tab-pages" class="tab-content">
<div class="card">
<div class="card-title">Visual Grid Editor</div>
<div class="form-group mb-2">
<label class="form-label">Select Page</label>
<div style="display:flex;gap:8px;">
<select id="page-selector" onchange="switchPage()" style="flex:1;">
</select>
<button class="btn btn-primary" onclick="addNewPage()">+ Add Page</button>
<button class="btn btn-danger" onclick="deletePage()">🗑️ Delete</button>
</div>
</div>
<div class="form-group mb-2">
<label class="form-label">Page Nav Button Color <small>(leave empty for theme default)</small></label>
<input type="color" id="page-nav-color" placeholder="#FFA500"/>
<button class="btn btn-secondary btn-small" onclick="clearPageNavColor()" style="margin-top:4px;">Use Theme Default</button>
</div>
<div class="grid grid-2 mb-2">
<div class="form-group">
<label class="form-label">Grid Rows</label>
<select id="grid-rows" onchange="updateGridPreview()">
<option value="1">1 Row</option>
<option value="2" selected>2 Rows</option>
<option value="3">3 Rows</option>
<option value="4">4 Rows</option>
</select>
</div>
<div class="form-group">
<label class="form-label">Grid Columns</label>
<select id="grid-cols" onchange="updateGridPreview()">
<option value="1">1 Column</option>
<option value="2" selected>2 Columns</option>
<option value="3">3 Columns</option>
<option value="4">4 Columns</option>
</select>
</div>
</div>
<div id="grid-preview" class="grid-preview"></div>
<div class="text-center text-muted mt-2">
<small>Click cells to add/edit buttons. Changes saved when you click Save Configuration below.</small>
</div>
</div>
<div class="card">
<div class="card-title">Page Configuration</div>
<div class="form-group">
<label class="form-label">Header Title</label>
<input type="text" id="header-title" placeholder="Bronco Controls"/>
</div>
<div class="form-group">
<label class="form-label">Header Subtitle</label>
<input type="text" id="header-subtitle" placeholder="Web Configurator"/>
</div>
<div class="checkbox-wrapper">
<input type="checkbox" id="header-show-logo" checked/>
<label for="header-show-logo">Show Logo in Header</label>
</div>
<div class="checkbox-wrapper">
<input type="checkbox" id="header-show-clock" checked/>
<label for="header-show-clock">Show Clock</label>
</div>
<div class="form-group">
<label class="form-label">Custom Logo Upload <small>(PNG/JPG, max 100KB)</small></label>
<input type="file" id="logo-upload" accept="image/png,image/jpeg,image/jpg" style="margin-bottom:8px;"/>
<button class="btn btn-secondary btn-small" onclick="clearCustomLogo()">Reset to Default Logo</button>
<div id="logo-preview" style="margin-top:12px;display:none;">
<img id="logo-preview-img" style="max-height:50px;border:1px solid var(--border);border-radius:4px;padding:4px;background:var(--surface);"/>
</div>
</div>
</div>
</div>
<div id="tab-can-library" class="tab-content">
<div class="card">
<div class="card-title">CAN Message Library</div>
<p class="text-muted mb-2">Create reusable CAN messages for quick button configuration.</p>
<button class="btn btn-primary mb-2" onclick="addCanMessage()">+ Add New Message</button>
<div id="can-library-list"></div>
</div>
<div class="card">
<div class="card-title">Quick Import Common Messages</div>
<div class="grid grid-3">
<button class="btn btn-secondary btn-small" onclick="importCanMessage('windows')">Windows</button>
<button class="btn btn-secondary btn-small" onclick="importCanMessage('locks')">Locks</button>
<button class="btn btn-secondary btn-small" onclick="importCanMessage('boards')">Running Boards</button>
</div>
</div>
</div>
<div class="card">
<div class="flex flex-between flex-center">
<div>
<button class="btn btn-success" onclick="saveConfig()">💾 Save Configuration</button>
<button class="btn btn-secondary" onclick="loadConfig()">🔄 Reload</button>
</div>
<div class="text-muted">
<small>Last saved: <span id="last-saved">Never</span></small>
</div>
</div>
</div>
</div>
<div id="button-modal" class="modal">
<div class="modal-content">
<div class="modal-header">
<span class="modal-title">Edit Button</span>
<button class="close-btn" onclick="closeModal()">&times;</button>
</div>
<div class="form-group">
<label class="form-label">Button Label</label>
<input type="text" id="btn-label" placeholder="My Button"/>
</div>
<div class="form-group">
<label class="form-label">Button Color</label>
<input type="color" id="btn-color" value="#FFA500"/>
</div>
<div class="form-group">
<label class="form-label">Button Pressed Color</label>
<input type="color" id="btn-pressed-color" value="#FF8800"/>
</div>
<div class="form-group">
<label class="form-label">Icon</label>
<select id="btn-icon">
<option value="">None</option>
<option value="home">🏠 Home</option>
<option value="windows">🪟 Windows</option>
<option value="locks">🔒 Locks</option>
<option value="boards">📋 Boards</option>
<option value="lights">💡 Lights</option>
<option value="aux">⚡ Aux</option>
<option value="settings">⚙️ Settings</option>
<option value="info">ℹ️ Info</option>
</select>
</div>
<div class="form-group">
<label class="form-label">Font Size (px)</label>
<input type="number" id="btn-font-size" value="24" min="8" max="72"/>
<small style="display:block;color:#888;margin-top:4px;">12, 14, 16, 18, 20, 22, 24, 28, 32 available</small>
</div>
<div class="form-group">
<label class="form-label">Text Alignment</label>
<select id="btn-text-align" style="width:100%;padding:8px;background:#2a2a2a;border:1px solid #444;color:#fff;border-radius:6px;">
<option value="top-left">Top Left</option>
<option value="top-center">Top Center</option>
<option value="top-right">Top Right</option>
<option value="center" selected>Center</option>
<option value="bottom-left">Bottom Left</option>
<option value="bottom-center">Bottom Center</option>
<option value="bottom-right">Bottom Right</option>
</select>
</div>
<div class="form-group">
<label class="form-label">Font Family</label>
<select id="btn-font-family" style="width:100%;padding:8px;background:#2a2a2a;border:1px solid #444;color:#fff;border-radius:6px;">
<option value="montserrat">Montserrat (Default)</option>
</select>
<small style="display:block;color:#888;margin-top:4px;">More fonts coming in future updates</small>
</div>
<div class="checkbox-wrapper">
<input type="checkbox" id="btn-momentary"/>
<label for="btn-momentary">Momentary (auto-release)</label>
</div>
<hr style="margin:24px 0;border:none;border-top:1px solid #333;"/>
<div style="margin-bottom:16px;">
<div style="font-weight:600;color:#ddd;margin-bottom:8px;">CAN Configuration</div>
<div class="checkbox-wrapper" style="margin-bottom:12px;">
<input type="checkbox" id="btn-can-enabled"/>
<label for="btn-can-enabled">Send CAN Message on Press</label>
</div>
<div id="can-library-selector" style="display:none;margin-bottom:16px;">
<div class="form-group">
<label class="form-label">Load from CAN Library</label>
<select id="btn-can-library-select" onchange="loadCanFromLibrary()">
<option value="">-- Select a message --</option>
</select>
<small class="text-muted" style="display:block;margin-top:4px;">Or configure manually below</small>
</div>
</div>
<div id="can-config-fields" style="display:none;">
<div class="grid grid-2" style="gap:12px;">
<div class="form-group">
<label class="form-label">PGN (hex)</label>
<input type="text" id="btn-can-pgn" placeholder="FEF9" maxlength="8"/>
</div>
<div class="form-group">
<label class="form-label">Priority</label>
<input type="number" id="btn-can-priority" value="6" min="0" max="7"/>
</div>
<div class="form-group">
<label class="form-label">Source Address (hex)</label>
<input type="text" id="btn-can-src" placeholder="F9" maxlength="2"/>
</div>
<div class="form-group">
<label class="form-label">Dest Address (hex)</label>
<input type="text" id="btn-can-dest" placeholder="FF" maxlength="2"/>
</div>
</div>
<div class="form-group">
<label class="form-label">Data Bytes (8 hex bytes, space-separated)</label>
<input type="text" id="btn-can-data" placeholder="00 01 02 03 04 05 06 07" maxlength="23"/>
</div>
</div>
</div>
<div style="display:flex;gap:12px;margin-top:24px;">
<button class="btn btn-primary" onclick="saveButton()" style="flex:1">💾 Save Button</button>
<button class="btn btn-danger" onclick="deleteButton()" style="flex:1">🗑️ Delete</button>
<button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
</div>
</div>
</div>
<script>
let config={};
let wifiNetworks=[];
let selectedWiFi=null;
let editingButton={row:-1,col:-1};
function switchTab(tabId){
document.querySelectorAll('.tab-btn').forEach(btn=>btn.classList.remove('active'));
document.querySelectorAll('.tab-content').forEach(content=>content.classList.remove('active'));
event.target.classList.add('active');
document.getElementById('tab-'+tabId).classList.add('active');
}
function showAlert(message,type='success'){
const alert=document.getElementById('status-alert');
alert.className=`alert alert-${type}`;
alert.innerHTML=`<span style="font-size:1.2rem;">${type==='success'?'✓':'⚠'}</span> ${message}`;
alert.style.display='flex';
setTimeout(()=>alert.style.display='none',4000);
}
async function scanWiFi(){
const btn=event.target;
const icon=document.getElementById('scan-icon');
icon.innerHTML='<div class="spinner"></div>';
btn.disabled=true;
try{
const response=await fetch('/api/wifi/scan');
const data=await response.json();
wifiNetworks=data.networks||[];
displayWiFiList();
showAlert('Found '+wifiNetworks.length+' networks','success');
}catch(error){
showAlert('Scan failed: '+error.message,'danger');
}finally{
icon.innerHTML='🔍';
btn.disabled=false;
}
}
function displayWiFiList(){
const list=document.getElementById('wifi-list');
if(wifiNetworks.length===0){
list.style.display='none';
return;
}
list.style.display='block';
list.innerHTML=wifiNetworks.map((net,idx)=>`
<div class="wifi-item" onclick="selectWiFi(${idx})">
<div>
<div class="wifi-name">${net.ssid}</div>
<div class="wifi-signal">
Channel ${net.channel||'?'}
<span class="wifi-strength">${getSignalBars(net.rssi)}</span>
</div>
</div>
<div>${net.secure?'🔒':'🔓'}</div>
</div>
`).join('');
}
function selectWiFi(index){
selectedWiFi=wifiNetworks[index];
document.getElementById('sta-ssid').value=selectedWiFi.ssid;
document.getElementById('sta-password').focus();
document.querySelectorAll('.wifi-item').forEach((item,idx)=>{
item.classList.toggle('selected',idx===index);
});
}
function getSignalBars(rssi){
if(rssi>-50)return '▰▰▰▰';
if(rssi>-60)return '▰▰▰▱';
if(rssi>-70)return '▰▰▱▱';
if(rssi>-80)return '▰▱▱▱';
return '▱▱▱▱';
}
function applyThemePreview(){
const root=document.documentElement;
root.style.setProperty('--bg',document.getElementById('theme-bg').value);
root.style.setProperty('--panel',document.getElementById('theme-surface').value);
root.style.setProperty('--accent',document.getElementById('theme-accent').value);
root.style.setProperty('--text',document.getElementById('theme-text-primary').value);
root.style.setProperty('--text-dim',document.getElementById('theme-text-secondary').value);
root.style.setProperty('--border',document.getElementById('theme-border').value);
root.style.setProperty('--surface',document.getElementById('theme-nav-button').value);
const preview=document.getElementById('grid-preview');
if(preview)preview.style.background=document.getElementById('theme-page-bg').value;
showAlert('Theme preview applied to web interface!','success');
}
let activePageIndex=0;
function updateGridPreview(){
const rows=parseInt(document.getElementById('grid-rows').value);
const cols=parseInt(document.getElementById('grid-cols').value);
const preview=document.getElementById('grid-preview');
preview.style.gridTemplateRows=`repeat(${rows},1fr)`;
preview.style.gridTemplateColumns=`repeat(${cols},1fr)`;
preview.innerHTML='';
if(!config.pages||config.pages.length===0){
config.pages=[{id:'page_0',name:'Home',rows:rows,cols:cols,buttons:[]}];
activePageIndex=0;
}
if(activePageIndex>=config.pages.length)activePageIndex=0;
const currentPage=config.pages[activePageIndex];
currentPage.rows=rows;
currentPage.cols=cols;
for(let r=0;r<rows;r++){
for(let c=0;c<cols;c++){
const button=currentPage.buttons.find(b=>b.row===r&&b.col===c);
if(button){
const cell=document.createElement('div');
cell.className='grid-preview-btn';
cell.style.gridRow=`${r+1} / span ${button.row_span||1}`;
cell.style.gridColumn=`${c+1} / span ${button.col_span||1}`;
cell.style.background=button.color||'var(--accent)';
cell.innerHTML=`
<div class="btn-icon">${getIconSymbol(button.icon)}</div>
<div>${button.label}</div>
`;
cell.onclick=()=>editButton(r,c);
preview.appendChild(cell);
}else{
const cell=document.createElement('div');
cell.className='grid-preview-empty';
cell.innerHTML='+';
cell.onclick=()=>addButton(r,c);
preview.appendChild(cell);
}
}
}
}
function getIconSymbol(iconName){
const icons={
'home':'🏠','windows':'🪟','locks':'🔒','boards':'📋',
'lights':'💡','aux':'⚡','settings':'⚙️','info':'ℹ️'
};
return icons[iconName]||'📱';
}
function addButton(row,col){
editingButton={row:row,col:col};
document.getElementById('btn-label').value='Button '+(row*10+col);
document.getElementById('btn-color').value='#FFA500';
document.getElementById('btn-pressed-color').value='#FF8800';
document.getElementById('btn-icon').value='';
document.getElementById('btn-font-size').value='24';
document.getElementById('btn-font-family').value='montserrat';
document.getElementById('btn-text-align').value='center';
document.getElementById('btn-momentary').checked=false;
document.getElementById('btn-can-enabled').checked=false;
document.getElementById('btn-can-pgn').value='FEF9';
document.getElementById('btn-can-priority').value='6';
document.getElementById('btn-can-src').value='F9';
document.getElementById('btn-can-dest').value='FF';
document.getElementById('btn-can-data').value='00 00 00 00 00 00 00 00';
document.getElementById('can-config-fields').style.display='none';
document.getElementById('can-library-selector').style.display='none';
populateCanLibraryDropdown();
document.getElementById('button-modal').style.display='block';
}
function editButton(row,col){
if(!config.pages||!config.pages[activePageIndex])return;
const button=config.pages[activePageIndex].buttons.find(b=>b.row===row&&b.col===col);
if(!button)return;
editingButton={row:row,col:col};
document.getElementById('btn-label').value=button.label||'';
document.getElementById('btn-color').value=button.color||'#FFA500';
document.getElementById('btn-pressed-color').value=button.pressed_color||'#FF8800';
document.getElementById('btn-icon').value=button.icon||'';
document.getElementById('btn-font-size').value=button.font_size||24;
document.getElementById('btn-font-family').value=button.font_family||'montserrat';
document.getElementById('btn-text-align').value=button.text_align||'center';
document.getElementById('btn-momentary').checked=button.momentary||false;
const canEnabled=button.can&&button.can.enabled||false;
document.getElementById('btn-can-enabled').checked=canEnabled;
if(button.can){
document.getElementById('btn-can-pgn').value=button.can.pgn.toString(16).toUpperCase();
document.getElementById('btn-can-priority').value=button.can.priority||6;
document.getElementById('btn-can-src').value=(button.can.source_address||0xF9).toString(16).toUpperCase();
document.getElementById('btn-can-dest').value=(button.can.destination_address||0xFF).toString(16).toUpperCase();
const dataStr=button.can.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
document.getElementById('btn-can-data').value=dataStr;
}
document.getElementById('can-config-fields').style.display=canEnabled?'block':'none';
document.getElementById('can-library-selector').style.display=canEnabled?'block':'none';
populateCanLibraryDropdown();
document.getElementById('button-modal').style.display='block';
}
function saveButton(){
if(!config.pages||config.pages.length===0){
config.pages=[{id:'page_0',name:'Home',rows:2,cols:2,buttons:[]}];
activePageIndex=0;
}
const page=config.pages[activePageIndex];
const existingIdx=page.buttons.findIndex(b=>b.row===editingButton.row&&b.col===editingButton.col);
const canEnabled=document.getElementById('btn-can-enabled').checked;
const canConfig={
enabled:canEnabled,
pgn:canEnabled?parseInt(document.getElementById('btn-can-pgn').value,16):0,
priority:canEnabled?parseInt(document.getElementById('btn-can-priority').value):6,
source_address:canEnabled?parseInt(document.getElementById('btn-can-src').value,16):0xF9,
destination_address:canEnabled?parseInt(document.getElementById('btn-can-dest').value,16):0xFF,
data:canEnabled?document.getElementById('btn-can-data').value.split(' ').map(b=>parseInt(b,16)||0):[0,0,0,0,0,0,0,0]
};
const buttonData={
id:'btn_'+editingButton.row+'_'+editingButton.col,
label:document.getElementById('btn-label').value,
color:document.getElementById('btn-color').value,
pressed_color:document.getElementById('btn-pressed-color').value,
icon:document.getElementById('btn-icon').value,
font_size:parseInt(document.getElementById('btn-font-size').value)||24,
font_family:document.getElementById('btn-font-family').value||'montserrat',
text_align:document.getElementById('btn-text-align').value||'center',
row:editingButton.row,
col:editingButton.col,
row_span:1,
col_span:1,
momentary:document.getElementById('btn-momentary').checked,
can:canConfig
};
if(existingIdx>=0){
page.buttons[existingIdx]=buttonData;
}else{
page.buttons.push(buttonData);
}
closeModal();
updateGridPreview();
showAlert('Button updated! Click Save Configuration to persist.','success');
}
function deleteButton(){
if(!config.pages||!config.pages[activePageIndex])return;
const page=config.pages[activePageIndex];
const idx=page.buttons.findIndex(b=>b.row===editingButton.row&&b.col===editingButton.col);
if(idx>=0){
page.buttons.splice(idx,1);
closeModal();
updateGridPreview();
showAlert('Button deleted! Click Save Configuration to persist.','success');
}
}
function closeModal(){
document.getElementById('button-modal').style.display='none';
}
function displayCanLibrary(){
const list=document.getElementById('can-library-list');
const messages=config.can_library||[];
if(messages.length===0){
list.innerHTML='<div class="text-center text-muted">No messages in library. Add one to get started!</div>';
return;
}
list.innerHTML=messages.map((msg,idx)=>`
<div class="can-message-item">
<div class="can-message-header">
<div>
<div class="can-message-name">${msg.name}</div>
<div class="can-message-pgn">PGN: 0x${msg.pgn.toString(16).toUpperCase().padStart(5,'0')}</div>
</div>
<div>
<button class="btn btn-danger btn-small" onclick="deleteCanMessage(${idx})">Delete</button>
</div>
</div>
${msg.description?`<div class="text-muted" style="font-size:.85rem;">${msg.description}</div>`:''}
<div class="can-message-data">
<div class="can-data-bytes">
${msg.data.map((b,i)=>`<span class="can-data-byte">${b.toString(16).toUpperCase().padStart(2,'0')}</span>`).join('')}
</div>
</div>
</div>
`).join('');
}
function addCanMessage(){
const name=prompt('Message Name:','New Message');
if(!name)return;
const pgn=parseInt(prompt('PGN (hex, e.g. FEF6):','FEF6'),16);
if(isNaN(pgn))return;
if(!config.can_library)config.can_library=[];
config.can_library.push({
id:'msg_'+Date.now(),
name:name,
pgn:pgn,
priority:6,
source_address:0xF9,
destination_address:0xFF,
data:[0,0,0,0,0,0,0,0],
description:''
});
displayCanLibrary();
showAlert('Message added! Click Save Configuration to persist.','success');
}
function deleteCanMessage(index){
if(confirm('Delete this message from library?')){
config.can_library.splice(index,1);
displayCanLibrary();
showAlert('Message deleted! Click Save Configuration to persist.','success');
}
}
function importCanMessage(type){
const templates={
'windows':{name:'Window Control',pgn:0xFEF6,data:[0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF]},
'locks':{name:'Door Locks',pgn:0xFECA,data:[0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF]},
'boards':{name:'Running Boards',pgn:0xFE00,data:[0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF]}
};
const template=templates[type];
if(!template)return;
if(!config.can_library)config.can_library=[];
config.can_library.push({
id:'msg_'+Date.now(),
name:template.name,
pgn:template.pgn,
priority:6,
source_address:0xF9,
destination_address:0xFF,
data:template.data,
description:'Pre-configured message'
});
displayCanLibrary();
showAlert(`Imported ${template.name}! Click Save Configuration to persist.`,'success');
}
async function loadConfig(){
try{
const response=await fetch('/api/config');
config=await response.json();
document.getElementById('ap-ssid').value=config.wifi?.ap?.ssid||'BroncoControls';
document.getElementById('ap-password').value=config.wifi?.ap?.password||'';
document.getElementById('ap-enabled').checked=config.wifi?.ap?.enabled!==false;
document.getElementById('sta-ssid').value=config.wifi?.sta?.ssid||'';
document.getElementById('sta-password').value=config.wifi?.sta?.password||'';
document.getElementById('sta-enabled').checked=config.wifi?.sta?.enabled||false;
const theme=config.theme||{};
document.getElementById('theme-bg').value=theme.bg_color||'#1A1A1A';
document.getElementById('theme-surface').value=theme.surface_color||'#2A2A2A';
document.getElementById('theme-page-bg').value=theme.page_bg_color||'#0F0F0F';
document.getElementById('theme-accent').value=theme.accent_color||'#FFA500';
document.getElementById('theme-text-primary').value=theme.text_primary||'#FFFFFF';
document.getElementById('theme-text-secondary').value=theme.text_secondary||'#AAAAAA';
document.getElementById('theme-border').value=theme.border_color||'#3A3A3A';
document.getElementById('theme-header-border').value=theme.header_border_color||'#FFA500';
document.getElementById('theme-nav-button').value=theme.nav_button_color||'#3A3A3A';
document.getElementById('theme-nav-active').value=theme.nav_button_active_color||'#FFA500';
document.getElementById('theme-radius').value=theme.button_radius||12;
document.getElementById('theme-border-width').value=theme.border_width||2;
document.getElementById('theme-header-border-width').value=theme.header_border_width||0;
document.getElementById('header-title').value=config.header?.title||'Bronco Controls';
document.getElementById('header-subtitle').value=config.header?.subtitle||'Web Configurator';
document.getElementById('header-show-logo').checked=config.header?.show_logo!==false;
document.getElementById('header-show-clock').checked=config.header?.show_clock!==false;
if(config.header?.logo_base64){
document.getElementById('logo-preview').style.display='block';
document.getElementById('logo-preview-img').src=config.header.logo_base64;
}else{
document.getElementById('logo-preview').style.display='none';
}
populatePageSelector();
if(config.pages&&config.pages[activePageIndex]){
document.getElementById('grid-rows').value=config.pages[activePageIndex].rows||2;
document.getElementById('grid-cols').value=config.pages[activePageIndex].cols||2;
}
updateGridPreview();
displayCanLibrary();
showAlert('Configuration loaded','success');
}catch(error){
showAlert('Failed to load: '+error.message,'danger');
}
}
async function saveConfig(){
if(!config.version)config.version='1.0.0';
config.wifi={
ap:{
enabled:document.getElementById('ap-enabled').checked,
ssid:document.getElementById('ap-ssid').value,
password:document.getElementById('ap-password').value
},
sta:{
enabled:document.getElementById('sta-enabled').checked,
ssid:document.getElementById('sta-ssid').value,
password:document.getElementById('sta-password').value
}
};
config.theme={
bg_color:document.getElementById('theme-bg').value,
surface_color:document.getElementById('theme-surface').value,
page_bg_color:document.getElementById('theme-page-bg').value,
accent_color:document.getElementById('theme-accent').value,
text_primary:document.getElementById('theme-text-primary').value,
text_secondary:document.getElementById('theme-text-secondary').value,
border_color:document.getElementById('theme-border').value,
header_border_color:document.getElementById('theme-header-border').value,
nav_button_color:document.getElementById('theme-nav-button').value,
nav_button_active_color:document.getElementById('theme-nav-active').value,
button_radius:parseInt(document.getElementById('theme-radius').value),
border_width:parseInt(document.getElementById('theme-border-width').value),
header_border_width:parseInt(document.getElementById('theme-header-border-width').value)
};
config.header={
title:document.getElementById('header-title').value,
subtitle:document.getElementById('header-subtitle').value,
show_logo:document.getElementById('header-show-logo').checked,
show_clock:document.getElementById('header-show-clock').checked,
logo_variant:'bronco',
logo_base64:config.header?.logo_base64||'',
title_font:config.header?.title_font||'montserrat_24',
subtitle_font:config.header?.subtitle_font||'montserrat_12'
};
if(!config.pages||config.pages.length===0){
config.pages=[{
id:'page_0',
name:'Home',
rows:parseInt(document.getElementById('grid-rows').value),
cols:parseInt(document.getElementById('grid-cols').value),
buttons:[]
}];
activePageIndex=0;
}else if(activePageIndex<config.pages.length){
config.pages[activePageIndex].rows=parseInt(document.getElementById('grid-rows').value);
config.pages[activePageIndex].cols=parseInt(document.getElementById('grid-cols').value);
const pageNavColor=document.getElementById('page-nav-color').value;
config.pages[activePageIndex].nav_color=(pageNavColor&&pageNavColor!=='#3A3A3A')?pageNavColor:'';
}
try{
const response=await fetch('/api/config',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify(config)
});
if(response.ok){
showAlert('✓ Configuration saved! Restart ESP32 device to see theme changes on display.','success');
document.getElementById('last-saved').textContent=new Date().toLocaleTimeString();
}else{
const text=await response.text();
throw new Error('Server error: '+text);
}
}catch(error){
showAlert('Failed to save: '+error.message,'danger');
}
}
window.addEventListener('DOMContentLoaded',()=>{
loadConfig();
document.getElementById('btn-can-enabled').addEventListener('change',function(){
const isEnabled=this.checked;
document.getElementById('can-config-fields').style.display=isEnabled?'block':'none';
document.getElementById('can-library-selector').style.display=isEnabled?'block':'none';
});
document.getElementById('logo-upload').addEventListener('change',handleLogoUpload);
});
function handleLogoUpload(event){
const file=event.target.files[0];
if(!file)return;
if(file.size>512000){
showAlert('Logo file too large! Max 500KB','danger');
event.target.value='';
return;
}
if(!file.type.match('image/(png|jpeg|jpg)')){
showAlert('Only PNG and JPG files are supported','danger');
event.target.value='';
return;
}
const reader=new FileReader();
reader.onload=function(e){
const img=new Image();
img.onload=function(){
const tempCanvas=document.createElement('canvas');
let width=img.width;
let height=img.height;
const maxWidth=48;
const maxHeight=36;
if(width>maxWidth||height>maxHeight){
const ratio=Math.min(maxWidth/width,maxHeight/height);
width=Math.floor(width*ratio);
height=Math.floor(height*ratio);
}
tempCanvas.width=width;
tempCanvas.height=height;
const tempCtx=tempCanvas.getContext('2d');
tempCtx.imageSmoothingEnabled=true;
tempCtx.imageSmoothingQuality='high';
tempCtx.drawImage(img,0,0,width,height);
const imgData=tempCtx.getImageData(0,0,width,height);
const data=imgData.data;
const tolerance=30;
for(let i=0;i<data.length;i+=4){
const r=data[i];
const g=data[i+1];
const b=data[i+2];
if(r>255-tolerance&&g>255-tolerance&&b>255-tolerance){
data[i+3]=0;
}
}
tempCtx.putImageData(imgData,0,0);
const canvas=document.createElement('canvas');
canvas.width=width;
canvas.height=height;
const ctx=canvas.getContext('2d');
const headerBg=config.theme?.bg_header||'#2A2A2A';
ctx.fillStyle=headerBg;
ctx.fillRect(0,0,width,height);
ctx.drawImage(tempCanvas,0,0);
let quality=0.8;
let base64=canvas.toDataURL('image/png',quality);
while(base64.length>10000&&quality>0.3){
quality-=0.1;
base64=canvas.toDataURL('image/png',quality);
}
if(base64.length>10000){
showAlert('Logo too large ('+Math.round(base64.length/1024)+'KB). Try a simpler image.','danger');
event.target.value='';
return;
}
if(!config.header)config.header={};
config.header.logo_base64=base64;
document.getElementById('logo-preview').style.display='block';
document.getElementById('logo-preview-img').src=base64;
showAlert('Logo uploaded ('+Math.round(base64.length/1024)+'KB)! Save to apply.','success');
};
img.src=e.target.result;
};
reader.readAsDataURL(file);
}
function clearCustomLogo(){
if(!config.header)config.header={};
config.header.logo_base64='';
document.getElementById('logo-upload').value='';
document.getElementById('logo-preview').style.display='none';
showAlert('Reset to default logo. Save configuration to apply.','success');
}
function populateCanLibraryDropdown(){
const select=document.getElementById('btn-can-library-select');
select.innerHTML='<option value="">-- Select a message --</option>';
const messages=config.can_library||[];
messages.forEach((msg,idx)=>{
const option=document.createElement('option');
option.value=idx;
option.textContent=msg.name+' (PGN: 0x'+msg.pgn.toString(16).toUpperCase()+')';
select.appendChild(option);
});
}
function loadCanFromLibrary(){
const select=document.getElementById('btn-can-library-select');
const idx=select.value;
if(idx==='')return;
const msg=config.can_library[parseInt(idx)];
if(!msg)return;
document.getElementById('btn-can-pgn').value=msg.pgn.toString(16).toUpperCase();
document.getElementById('btn-can-priority').value=msg.priority;
document.getElementById('btn-can-src').value=msg.source_address.toString(16).toUpperCase();
document.getElementById('btn-can-dest').value=msg.destination_address.toString(16).toUpperCase();
const dataStr=msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
document.getElementById('btn-can-data').value=dataStr;
showAlert('Loaded CAN message: '+msg.name,'success');
}
function populatePageSelector(){
const selector=document.getElementById('page-selector');
selector.innerHTML='';
if(!config.pages||config.pages.length===0){
config.pages=[{id:'page_0',name:'Home',rows:2,cols:2,buttons:[]}];
}
config.pages.forEach((page,idx)=>{
const option=document.createElement('option');
option.value=idx;
option.textContent=page.name||'Page '+(idx+1);
if(idx===activePageIndex)option.selected=true;
selector.appendChild(option);
});
}
function switchPage(){
const selector=document.getElementById('page-selector');
activePageIndex=parseInt(selector.value)||0;
if(config.pages&&config.pages[activePageIndex]){
document.getElementById('grid-rows').value=config.pages[activePageIndex].rows||2;
document.getElementById('grid-cols').value=config.pages[activePageIndex].cols||2;
const navColor=config.pages[activePageIndex].nav_color||'';
if(navColor){
document.getElementById('page-nav-color').value=navColor;
}else{
document.getElementById('page-nav-color').value='#3A3A3A';
}
}
updateGridPreview();
}
function addNewPage(){
if(!config.pages)config.pages=[];
const newId='page_'+config.pages.length;
const newName=prompt('Enter page name:','Page '+(config.pages.length+1));
if(!newName)return;
config.pages.push({id:newId,name:newName,rows:2,cols:2,buttons:[]});
activePageIndex=config.pages.length-1;
populatePageSelector();
switchPage();
showAlert('Page added! Remember to save configuration.','success');
}
function deletePage(){
if(!config.pages||config.pages.length<=1){
showAlert('Cannot delete the last page!','danger');
return;
}
if(!confirm('Delete page "'+config.pages[activePageIndex].name+'"?'))return;
config.pages.splice(activePageIndex,1);
if(activePageIndex>=config.pages.length)activePageIndex=config.pages.length-1;
populatePageSelector();
switchPage();
showAlert('Page deleted! Remember to save configuration.','success');
}
function clearPageNavColor(){
if(!config.pages||!config.pages[activePageIndex])return;
config.pages[activePageIndex].nav_color='';
document.getElementById('page-nav-color').value='#3A3A3A';
showAlert('Page will use theme default nav color','success');
}
</script>
</body>
</html>
)rawliteral";
