#pragma once

const char WEB_INTERFACE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>CAN controls configurator</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@400;600;700&display=swap');
:root {
	--bg: #0b0c10;
	--panel: #12141c;
	--surface: #191c26;
	--accent: #ff9d2e;
	--accent-2: #7ad7f0;
	--text: #f2f4f8;
	--muted: #8d92a3;
	--border: #20232f;
	--success: #3dd598;
	--danger: #ff6b6b;
}
* { box-sizing: border-box; }
body {
	margin: 0;
	font-family: 'Space Grotesk', 'Segoe UI', sans-serif;
	background: radial-gradient(circle at 10% 10%, rgba(255,157,46,0.12), transparent 40%),
							radial-gradient(circle at 90% 10%, rgba(122,215,240,0.12), transparent 40%),
							var(--bg);
	color: var(--text);
	min-height: 100vh;
}
.hero {
	padding: 32px 4vw 16px;
	border-bottom: 1px solid var(--border);
	display: flex;
	justify-content: space-between;
	align-items: flex-start;
	gap: 12px;
	flex-wrap: wrap;
}
.hero h1 { margin: 0; letter-spacing: 0.08em; font-size: 2rem; line-height: 1.1; max-width: min(460px, 70vw); overflow-wrap: anywhere; }
.hero p { margin: 6px 0 0; color: var(--muted); }
.container { padding: 0 4vw 48px; }
.tabs { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 16px; }
.tab-btn {
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--muted);
	padding: 10px 14px;
	border-radius: 12px;
	cursor: pointer;
	transition: all .15s ease;
	font-weight: 600;
}
.tab-btn.active { background: var(--accent); color: #16110a; border-color: var(--accent); }
.tab-btn:hover { border-color: var(--accent); color: var(--text); }
.status-banner { display: none; padding: 12px 14px; border-radius: 12px; margin-bottom: 16px; }
.status-success { background: rgba(61,213,152,0.12); border: 1px solid var(--success); color: var(--success); }
.status-error { background: rgba(255,107,107,0.12); border: 1px solid var(--danger); color: var(--danger); }
.layout { display: grid; gap: 16px; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
.card {
	background: var(--panel);
	border: 1px solid var(--border);
	border-radius: 18px;
	padding: 18px 18px 14px;
	box-shadow: 0 18px 50px rgba(0,0,0,0.35);
}
.card h3 { margin: 0 0 10px; font-size: 1.05rem; display: flex; align-items: center; gap: 8px; letter-spacing: 0.03em; }
.card h4 { margin: 16px 0 8px; color: var(--muted); font-size: 0.9rem; letter-spacing: 0.02em; }
.muted { color: var(--muted); font-size: 0.9rem; }
.grid { display: grid; gap: 12px; }
.two-col { grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); }
.three-col { grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); }
label { color: var(--muted); font-size: 0.85rem; letter-spacing: 0.01em; }
input, select, textarea {
	width: 100%;
	padding: 10px 12px;
	border-radius: 12px;
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	font-size: 0.95rem;
}
input[type=color] { padding: 6px; height: 46px; }
input:focus, select:focus, textarea:focus { outline: 2px solid var(--accent); border-color: var(--accent); }
.row { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }
.row > * { flex: 1; }
.btn {
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	padding: 10px 14px;
	border-radius: 12px;
	cursor: pointer;
	font-weight: 700;
	transition: all .15s ease;
}
.btn.primary { background: var(--accent); color: #16110a; border-color: var(--accent); }
.btn.ghost { background: transparent; }
.btn.danger { background: var(--danger); border-color: var(--danger); color: #fff; }
.btn.small { padding: 8px 12px; font-size: 0.9rem; }
.pill { padding: 4px 10px; border-radius: 999px; border: 1px solid var(--border); color: var(--muted); font-size: 0.8rem; }
.pill.success { border-color: var(--success); color: var(--success); }
.pill.warn { border-color: var(--accent); color: var(--accent); }
.tab { display: none; }
.tab.active { display: block; animation: fade .25s ease; }
@keyframes fade { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }
.panel-split { display: grid; gap: 16px; grid-template-columns: minmax(320px, 380px) 1fr; }
.page-list { display: flex; flex-direction: column; gap: 8px; }
.page-chip { padding: 12px; border: 1px solid var(--border); border-radius: 14px; background: var(--surface); display: flex; justify-content: space-between; align-items: center; cursor: grab; }
.page-chip.active { border-color: var(--accent); box-shadow: 0 0 0 1px rgba(255,157,46,0.4); }
.page-chip .name { font-weight: 600; }
.preview-shell { border: 1px solid var(--border); border-radius: 18px; background: linear-gradient(135deg, rgba(255,157,46,0.05), rgba(122,215,240,0.05)); padding: 14px; }
.device-preview { border-radius: 16px; overflow: hidden; border: 1px solid var(--border); background: var(--bg); box-shadow: inset 0 0 0 1px rgba(255,255,255,0.02); padding-top: 8px; box-sizing: border-box; }

/* Revert to original hero header styling */
.hero {
	padding: 56px 4vw 16px; /* Increased top padding to prevent title from running off */
	border-bottom: 1px solid var(--border);
	display: flex;
	justify-content: space-between;
	align-items: flex-end; /* Ensure content is aligned to the bottom of the header area */
	gap: 12px;
	flex-wrap: wrap;
}
.hero h1 {
	margin: 0;
	letter-spacing: 0.08em;
	font-size: 2rem;
	line-height: 1.1;
	max-width: min(460px, 70vw);
	overflow-wrap: anywhere;
}
.hero p {
	margin: 6px 0 0;
	color: var(--muted);
}
.preview-nav { display: flex; gap: 8px; padding: 10px 12px; flex-wrap: wrap; background: var(--panel); border-bottom: 1px solid var(--border); }
.preview-nav .pill { cursor: grab; }
.preview-body { padding: 14px; min-height: 220px; }
.preview-grid { display: grid; gap: 10px; }
.preview-btn { padding: 14px 12px; border-radius: 12px; font-weight: 700; text-align: center; border: 1px solid transparent; cursor: pointer; }
.preview-btn.empty { border: 1px dashed var(--border); background: rgba(255,255,255,0.03); color: var(--muted); }
.quick-edit { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 10px; align-items: center; }
.quick-edit .field { display: inline-flex; align-items: center; gap: 6px; }
.quick-edit input[type="color"] { width: 46px; padding: 0; }
.quick-edit input[type="text"] { width: 150px; }
.builder-grid { border: 1px dashed var(--border); border-radius: 14px; padding: 12px; background: rgba(255,255,255,0.02); }
.grid-cell { border: 1px dashed var(--border); border-radius: 10px; min-height: 72px; display: flex; align-items: center; justify-content: center; color: var(--muted); cursor: pointer; transition: all .12s ease; }
.grid-cell:hover { border-color: var(--accent); color: var(--accent); background: rgba(255,157,46,0.06); }
.grid-btn { width: 100%; height: 100%; border-radius: 12px; padding: 10px; border: 1px solid var(--border); display: flex; align-items: center; justify-content: center; cursor: grab; }
.floating-bar { position: sticky; bottom: 0; margin-top: 18px; padding: 12px; background: rgba(11,12,16,0.7); backdrop-filter: blur(8px); border: 1px solid var(--border); border-radius: 14px; display: flex; justify-content: space-between; align-items: center; gap: 12px; box-shadow: 0 20px 40px rgba(0,0,0,0.35); }
.modal { position: fixed; inset: 0; display: none; background: rgba(0,0,0,0.6); align-items: center; justify-content: center; padding: 24px; z-index: 20; }
.modal.open { display: flex; }
.modal-content { background: var(--panel); border: 1px solid var(--border); border-radius: 16px; padding: 18px; width: min(680px, 96vw); max-height: 90vh; overflow: auto; box-shadow: 0 24px 60px rgba(0,0,0,0.45); }
.modal-head { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
.wifi-list { margin-top: 10px; display: grid; gap: 8px; }
.wifi-item { padding: 12px; border: 1px solid var(--border); border-radius: 12px; background: var(--surface); display: flex; justify-content: space-between; cursor: pointer; }
.wifi-item.active { border-color: var(--accent); }
.can-card { border: 1px solid var(--border); border-radius: 12px; padding: 12px; background: var(--surface); }
</style>
</head>
<body>
<div class="hero">
	<div>
		<h1>CAN controls configurator</h1>
		<p>Build, preview, and save the exact UI layout before flashing.</p>
	</div>
	<div class="pill">Live builder</div>
</div>
<div class="container">
	<div class="tabs">
		<button class="tab-btn active" data-tab="wifi" onclick="switchTab('wifi')">WiFi</button>
		<button class="tab-btn" data-tab="builder" onclick="switchTab('builder')">Interface Builder</button>
		<button class="tab-btn" data-tab="can" onclick="switchTab('can')">CAN Library</button>
	</div>
	<div id="status-banner" class="status-banner"></div>

	<section id="tab-wifi" class="tab active">
		<div class="layout">
			<div class="card">
				<h3>Access Point</h3>
				<div class="muted">Always-on local network to reach the configurator.</div>
				<div class="grid">
					<div>
						<label>SSID</label>
						<input id="ap-ssid" type="text" placeholder="CAN-Control" />
					</div>
					<div>
						<label>Password (8+ chars)</label>
						<input id="ap-password" type="password" placeholder="********" />
					</div>
					<div class="row">
						<label><input id="ap-enabled" type="checkbox" /> Enable AP</label>
					</div>
				</div>
			</div>
			<div class="card">
				<h3>Join Existing WiFi</h3>
				<div class="row">
					<button class="btn" onclick="scanWiFi()" id="scan-btn">Scan</button>
					<span class="pill warn">Choose, then save</span>
				</div>
				<div class="wifi-list" id="wifi-results"></div>
				<div class="grid">
					<div>
						<label>SSID</label>
						<input id="sta-ssid" type="text" placeholder="Your WiFi" />
					</div>
					<div>
						<label>Password</label>
						<input id="sta-password" type="password" placeholder="********" />
					</div>
					<div class="row">
						<label><input id="sta-enabled" type="checkbox" /> Connect on boot</label>
					</div>
				</div>
			</div>
		</div>
	</section>

	<section id="tab-builder" class="tab">
		<div class="panel-split">
			<div class="card">
				<h3>Branding & Header</h3>
				<div class="grid">
					<div class="row">
						<label>Title</label>
						<input id="header-title-input" type="text" placeholder="CAN Control" oninput="updateHeaderFromInputs()" />
						<label>Title Text</label>
						<input id="theme-text-primary" type="color" />
					</div>
					<div class="row">
						<label>Subtitle</label>
						<input id="header-subtitle-input" type="text" placeholder="Configuration Interface" oninput="updateHeaderFromInputs()" />
						<label>Subtitle Text</label>
						<input id="theme-text-secondary" type="color" />
					</div>
					<div class="row">
						<label>Title Font</label>
						<select id="header-title-font" onchange="updateHeaderFromInputs()"></select>
						<label>Subtitle Font</label>
						<select id="header-subtitle-font" onchange="updateHeaderFromInputs()"></select>
					</div>
					<div class="row">
						<label><input id="header-show-logo" type="checkbox" onchange="updateHeaderFromInputs()" /> Show logo</label>
						<input id="logo-upload" type="file" accept="image/png,image/jpeg" />
					</div>
					<div id="logo-preview" style="display:none;">
						<img id="logo-preview-img" style="max-height:48px; border:1px solid var(--border); border-radius:8px; padding:6px; background: var(--surface);" />
						<button class="btn small" onclick="clearCustomLogo()">Reset Logo</button>
					</div>
				</div>
				<h4>Theme Baseline</h4>
				<div class="grid two-col">
					<div><label>Header BG</label><input id="theme-surface" type="color" /></div>
					<div><label>Page BG</label><input id="theme-page-bg" type="color" /></div>
					<div><label>Border</label><input id="theme-border" type="color" /></div>
					<div><label>Active NAV</label><input id="theme-nav-active" type="color" /></div>
					<div><label>Inactive NAV</label><input id="theme-nav-button" type="color" /></div>
					<div><label>Button Radius (default)</label><input id="theme-radius" type="number" min="0" max="50" /></div>
					<div><label>Border Width (default)</label><input id="theme-border-width" type="number" min="0" max="10" /></div>
					<div><label>Header Border Width</label><input id="theme-header-border-width" type="number" min="0" max="10" /></div>
					<div><label>Header Border</label><input id="theme-header-border" type="color" /></div>
				</div>
			</div>

			<div class="preview-shell">
				<div class="quick-edit">
					<div class="field"><label>Window</label><select id="quick-page-select" onchange="quickPageSelectChanged()"></select></div>
					<button class="btn small" onclick="addPage()">Add</button>
					<button class="btn small" onclick="deletePage()">Delete</button>
					<div class="field" style="min-width:140px;"><label>Grid</label>
						<select id="page-rows" onchange="updateGrid()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select>
						<span style="color:var(--muted);">x</span>
						<select id="page-cols" onchange="updateGrid()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select>
					</div>
					<div class="field"><label>Name</label><input id="page-name-input" type="text" oninput="updatePageMeta()" /></div>
					<div class="field"><label>Nav</label><input id="page-nav-color" type="color" onchange="updatePageMeta()" /></div>
					<div class="field"><label>Page BG</label><input id="page-bg-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Text</label><input id="page-text-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Button</label><input id="page-btn-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Pressed</label><input id="page-btn-pressed" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Border</label><input id="page-btn-border" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Border W</label><input id="page-btn-border-width" type="number" min="0" max="10" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Radius</label><input id="page-btn-radius" type="number" min="0" max="50" onchange="updatePageStyle()" /></div>
					<button class="btn small" onclick="applyPageStyleToButtons()">Apply to buttons</button>
					<button class="btn small ghost" onclick="capturePageAsBaseline()">Save as baseline</button>
					<button class="btn small ghost" onclick="applyBaselineToPage()">Apply baseline</button>
				</div>
				<div class="device-preview" id="live-preview">
					<div class="preview-header" id="preview-header">
						<div style="width:34px;height:34px; border-radius:10px; background: var(--accent);" id="preview-logo"></div>
						<div class="title-wrap" style="flex:1;">
							<div id="preview-title">CAN Control</div>
							<div id="preview-subtitle" class="muted" style="margin-top:4px;">Configuration Interface</div>
						</div>
					</div>
					<div class="preview-nav" id="preview-nav"></div>
					<div class="preview-body" id="preview-body"></div>
				</div>
			</div>
		</div>

		<div class="layout" style="margin-top:16px;">
			<div class="card">
				<h3>Display & Sleep</h3>
				<div class="grid two-col">
					<div><label>Brightness</label><input id="display-brightness" type="range" min="0" max="100" oninput="document.getElementById('brightness-value').textContent=this.value+'%';" /></div>
					<div class="row" style="justify-content:space-between;"><span class="muted">Value</span><span id="brightness-value">100%</span></div>
					<div class="row"><label><input id="sleep-enabled" type="checkbox" /> Enable Sleep Overlay</label></div>
					<div><label>Sleep Timeout (s)</label><input id="sleep-timeout" type="number" min="5" max="3600" /></div>
					<div class="row"><label>Sleep Icon</label><input id="sleep-icon-upload" type="file" accept="image/png,image/jpeg" /></div>
					<div id="sleep-icon-preview" style="display:none;"><img id="sleep-icon-preview-img" style="max-height:60px; border:1px solid var(--border); border-radius:8px; padding:6px; background: var(--surface);" /></div>
					<button class="btn small" onclick="clearSleepIcon()">Clear Sleep Icon</button>
				</div>
			</div>
		</div>
	</section>

	<section id="tab-can" class="tab">
		<div class="layout">
			<div class="card">
				<h3>CAN Message Library</h3>
				<div class="row" style="margin-bottom:10px;">
					<button class="btn primary" onclick="addCanMessage()">Add Message</button>
				</div>
				<div id="can-library-list" class="grid"></div>
			</div>
			<div class="card">
				<h3>Quick Import</h3>
				<div class="row">
					<button class="btn" onclick="importCanMessage('windows')">Windows</button>
					<button class="btn" onclick="importCanMessage('locks')">Locks</button>
					<button class="btn" onclick="importCanMessage('boards')">Running Boards</button>
				</div>
			</div>
		</div>
	</section>

	<div class="floating-bar">
		<div class="muted">Save mirrors what you preview. Reload pulls the current device config.</div>
		<div class="row" style="flex:0 0 auto;">
			<button class="btn" onclick="loadConfig()">Reload</button>
			<button class="btn primary" onclick="saveConfig()">Save</button>
		</div>
	</div>
</div>

<div class="modal" id="button-modal">
	<div class="modal-content">
		<div class="modal-head">
			<h3>Edit Button</h3>
			<button class="btn ghost" onclick="closeModal()">Close</button>
		</div>
		<div class="grid two-col">
			<div><label>Label</label><input id="btn-label" type="text" /></div>
			<div><label>Font Size</label><select id="btn-font-size"><option value="12">12</option><option value="14">14</option><option value="16">16</option><option value="18">18</option><option value="20">20</option><option value="22">22</option><option value="24">24</option><option value="28">28</option><option value="32">32</option></select></div>
			<div><label>Font Family</label><select id="btn-font-family"><option value="montserrat">Montserrat</option><option value="unscii">UNSCII</option></select></div>
			<div><label>Text Align</label><select id="btn-text-align"><option value="center">Center</option><option value="top-left">Top Left</option><option value="top-center">Top Center</option><option value="top-right">Top Right</option><option value="bottom-left">Bottom Left</option><option value="bottom-center">Bottom Center</option><option value="bottom-right">Bottom Right</option></select></div>
			<div><label>Fill</label><input id="btn-color" type="color" /></div>
			<div><label>Pressed</label><input id="btn-pressed-color" type="color" /></div>
			<div><label>Border</label><input id="btn-border-color" type="color" /></div>
			<div><label>Border Width</label><input id="btn-border-width" type="number" min="0" max="10" /></div>
			<div><label>Corner Radius</label><input id="btn-corner-radius" type="number" min="0" max="50" /></div>
			<div class="row"><label><input id="btn-momentary" type="checkbox" /> Momentary</label></div>
		</div>
		<h4>CAN Frame</h4>
		<div class="row" style="margin-bottom:10px;"><label><input id="btn-can-enabled" type="checkbox" onchange="toggleCanFields()" /> Send CAN on press</label></div>
		<div id="can-config-wrapper" class="grid two-col" style="display:none;">
			<div><label>PGN (hex)</label><input id="btn-can-pgn" type="text" placeholder="FEF9" /></div>
			<div><label>Priority</label><input id="btn-can-priority" type="number" min="0" max="7" /></div>
			<div><label>Source (hex)</label><input id="btn-can-src" type="text" placeholder="F9" /></div>
			<div><label>Dest (hex)</label><input id="btn-can-dest" type="text" placeholder="FF" /></div>
			<div class="row" style="grid-column:1/-1;">
				<label>Data Bytes</label>
				<input id="btn-can-data" type="text" placeholder="00 00 00 00 00 00 00 00" />
			</div>
			<div class="row" style="grid-column:1/-1;">
				<label>From Library</label>
				<select id="btn-can-library-select" onchange="loadCanFromLibrary()"></select>
			</div>
		</div>
		<div class="row" style="margin-top:12px; justify-content:flex-end; gap:8px;">
			<button class="btn danger" onclick="deleteButtonFromModal()">Delete</button>
			<button class="btn primary" onclick="saveButtonFromModal()">Save</button>
		</div>
	</div>
</div>

<script>
let config = {};
let activePageIndex = 0;
let editingButton = { row: -1, col: -1 };
let wifiNetworks = [];

function firstDefined() {
	for (let i = 0; i < arguments.length; i++) {
		const v = arguments[i];
		if (v !== undefined && v !== null) {
			return v;
		}
	}
	return undefined;
}

function switchTab(tabName){
	document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));
	document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
	document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
	document.getElementById(`tab-${tabName}`).classList.add('active');
}

function showBanner(msg,type='success'){
	const el = document.getElementById('status-banner');
	el.className = `status-banner ${type==='success'?'status-success':'status-error'}`;
	el.textContent = msg;
	el.style.display = 'block';
	setTimeout(()=>{ el.style.display='none'; }, 3500);
}

function ensurePages(){
	if(!config.pages || config.pages.length===0){
		config.pages = [{ id:'page_0', name:'Home', rows:2, cols:2, buttons:[] }];
		activePageIndex = 0;
	}
}

function renderPageSelect(){
	const sel = document.getElementById('quick-page-select');
	if(!sel) return;
	sel.innerHTML = '';
	config.pages.forEach((page, idx)=>{
		const opt = document.createElement('option');
		opt.value = `${idx}`;
		opt.textContent = page.name || `Window ${idx+1}`;
		sel.appendChild(opt);
	});
	sel.value = `${activePageIndex}`;
}

function renderPageList(){
	ensurePages();
	renderPageSelect();
	const list = document.getElementById('page-list');
	if(!list) return;
	list.innerHTML = '';
	config.pages.forEach((page,idx)=>{
		const item = document.createElement('div');
		item.className = 'page-chip'+(idx===activePageIndex?' active':'');
		item.draggable = true;
		item.dataset.index = idx;
		item.innerHTML = `<span class="name">${page.name||'Page '+(idx+1)}</span><span class="muted">${page.rows}x${page.cols}</span>`;
		item.onclick = ()=>setActivePage(idx);
		item.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', idx); };
		item.ondragover = (e)=>{ e.preventDefault(); };
		item.ondrop = (e)=>{
			e.preventDefault();
			const from = parseInt(e.dataTransfer.getData('text/plain'));
			if(isNaN(from) || from===idx) return;
			const moved = config.pages.splice(from,1)[0];
			config.pages.splice(idx,0,moved);
			activePageIndex = idx;
			renderPageList();
			renderNav();
			renderGrid();
			renderPreview();
		};
		list.appendChild(item);
	});
}

function quickPageSelectChanged(){
	const sel = document.getElementById('quick-page-select');
	if(!sel) return;
	const idx = parseInt(sel.value);
	if(!isNaN(idx)) setActivePage(idx);
}

function setActivePage(idx){
	activePageIndex = idx;
	hydratePageFields();
	renderPageList();
	renderGrid();
	renderPreview();
}

function addPage(){
	ensurePages();
	const id = 'page_'+config.pages.length;
	config.pages.push({ id, name:'Page '+(config.pages.length+1), rows:2, cols:2, buttons:[] });
	activePageIndex = config.pages.length-1;
	renderPageList();
	hydratePageFields();
	renderGrid();
	renderPreview();
}

function deletePage(){
	ensurePages();
	if(config.pages.length<=1){ showBanner('At least one page is required','error'); return; }
	config.pages.splice(activePageIndex,1);
	if(activePageIndex>=config.pages.length) activePageIndex = config.pages.length-1;
	renderPageList();
	hydratePageFields();
	renderGrid();
	renderPreview();
}

function hydratePageFields(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const nameInput = document.getElementById('page-name-input');
	if(nameInput) nameInput.value = page.name || '';
	document.getElementById('page-nav-color').value = page.nav_color || '#3a3a3a';
	document.getElementById('page-bg-color').value = page.bg_color || theme.page_bg_color || '#0f0f0f';
	document.getElementById('page-text-color').value = page.text_color || theme.text_primary || '#ffffff';
	document.getElementById('page-btn-color').value = page.button_color || theme.button_color || theme.accent_color || '#ff9d2e';
	document.getElementById('page-btn-pressed').value = page.button_pressed_color || theme.button_pressed_color || '#ff7a1a';
	document.getElementById('page-btn-border').value = page.button_border_color || theme.border_color || '#3a3a3a';
	document.getElementById('page-btn-border-width').value = page.button_border_width || 0;
	document.getElementById('page-btn-radius').value = page.button_radius || theme.button_radius || 12;
	document.getElementById('page-rows').value = page.rows || 2;
	document.getElementById('page-cols').value = page.cols || 2;
	const sel = document.getElementById('quick-page-select');
	if(sel) sel.value = `${activePageIndex}`;
}

function updatePageMeta(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.name = document.getElementById('page-name-input').value || page.name;
	page.nav_color = document.getElementById('page-nav-color').value;
	renderPageList();
	renderNav();
	renderPreview();
}

function updatePageStyle(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.bg_color = document.getElementById('page-bg-color').value;
	page.text_color = document.getElementById('page-text-color').value;
	page.button_color = document.getElementById('page-btn-color').value;
	page.button_pressed_color = document.getElementById('page-btn-pressed').value;
	page.button_border_color = document.getElementById('page-btn-border').value;
	page.button_border_width = parseInt(document.getElementById('page-btn-border-width').value)||0;
	page.button_radius = parseInt(document.getElementById('page-btn-radius').value)||0;
	renderGrid();
	renderPreview();
}

function applyPageStyleToButtons(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.buttons = (page.buttons||[]).map(btn=>({
		...btn,
		color: page.button_color || btn.color,
		pressed_color: page.button_pressed_color || btn.pressed_color,
		border_color: page.button_border_color || btn.border_color,
		border_width: page.button_border_width || btn.border_width,
		corner_radius: page.button_radius || btn.corner_radius
	}));
	renderGrid();
	renderPreview();
	showBanner('Applied page styling to all buttons','success');
}

function capturePageAsBaseline(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const firstBtn = (page.buttons||[])[0] || {};
	const buttonFontFamily = firstDefined(firstBtn.font_family, theme.button_font_family, 'montserrat');
	const buttonFontSize = firstDefined(firstBtn.font_size, theme.button_font_size, 24);
	config.theme = {
		...theme,
		page_bg_color: page.bg_color || theme.page_bg_color || '#0f0f0f',
		text_primary: page.text_color || theme.text_primary || '#f2f4f8',
		accent_color: page.button_color || theme.accent_color || '#ff9d2e',
		button_pressed_color: page.button_pressed_color || theme.button_pressed_color || '#ff7a1a',
		border_color: page.button_border_color || theme.border_color || '#20232f',
		border_width: page.button_border_width || theme.border_width || 0,
		button_radius: page.button_radius || theme.button_radius || 12,
		button_font_family: buttonFontFamily,
		button_font_size: buttonFontSize
	};
	hydrateThemeFields();
	renderPreview();
	showBanner('Saved this page as the baseline theme','success');
}

function applyBaselineToPage(){
	ensurePages();
	const theme = config.theme || {};
	const page = config.pages[activePageIndex];
	page.bg_color = theme.page_bg_color || page.bg_color || '#0f0f0f';
	page.text_color = theme.text_primary || page.text_color || '#f2f4f8';
	page.button_color = theme.accent_color || page.button_color || '#ff9d2e';
	page.button_pressed_color = firstDefined(theme.button_pressed_color, page.button_pressed_color, '#ff7a1a');
	page.button_border_color = theme.border_color || page.button_border_color || '#20232f';
	page.button_border_width = theme.border_width !== undefined ? theme.border_width : (page.button_border_width || 0);
	page.button_radius = theme.button_radius !== undefined ? theme.button_radius : (page.button_radius || 12);
	// push updated fills/borders into existing buttons
	page.buttons = (page.buttons||[]).map(btn=>({
		...btn,
		color: theme.accent_color || page.button_color || btn.color,
		pressed_color: firstDefined(theme.button_pressed_color, page.button_pressed_color, btn.pressed_color, theme.accent_color, '#ff7a1a'),
		border_color: theme.border_color || page.button_border_color || btn.border_color,
		border_width: theme.border_width !== undefined ? theme.border_width : (page.button_border_width || btn.border_width),
		corner_radius: theme.button_radius !== undefined ? theme.button_radius : (page.button_radius || btn.corner_radius),
		font_family: theme.button_font_family || btn.font_family || 'montserrat',
		font_size: theme.button_font_size || btn.font_size || 24
	}));
	hydratePageFields();
	renderGrid();
	renderPreview();
	showBanner('Applied baseline to this page','success');
}

function updateGrid(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.rows = parseInt(document.getElementById('page-rows').value)||2;
	page.cols = parseInt(document.getElementById('page-cols').value)||2;
	renderGrid();
	renderPreview();
}

function renderGrid(){
	ensurePages();
	const grid = document.getElementById('layout-grid');
	if(!grid) return;
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	grid.style.gridTemplateColumns = `repeat(${page.cols}, minmax(120px, 1fr))`;
	grid.innerHTML = '';
	for(let r=0;r<page.rows;r++){
		for(let c=0;c<page.cols;c++){
			const btn = (page.buttons||[]).find(b=>b.row===r && b.col===c);
			if(btn){
				const cell = document.createElement('div');
				cell.className = 'grid-cell';
				const inner = document.createElement('div');
				inner.className = 'grid-btn';
				inner.style.background = btn.color || page.button_color || theme.button_color || theme.accent_color || '#ff9d2e';
				inner.style.color = '#000';
				inner.textContent = btn.label || 'Button';
				inner.draggable = true;
				inner.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', `${r},${c}`); };
				inner.ondrop = (e)=>{
					e.preventDefault();
					const [fr,fc] = e.dataTransfer.getData('text/plain').split(',').map(n=>parseInt(n));
					moveButton(fr,fc,r,c);
				};
				inner.ondragover = (e)=>e.preventDefault();
				inner.onclick = ()=>openButtonModal(r,c);
				cell.appendChild(inner);
				grid.appendChild(cell);
			} else {
				const empty = document.createElement('div');
				empty.className = 'grid-cell';
				empty.textContent = '+';
				empty.onclick = ()=>openButtonModal(r,c);
				grid.appendChild(empty);
			}
		}
	}
}

function moveButton(fr,fc,tr,tc){
	const page = config.pages[activePageIndex];
	const idx = (page.buttons||[]).findIndex(b=>b.row===fr && b.col===fc);
	if(idx<0) return;
	page.buttons[idx].row = tr;
	page.buttons[idx].col = tc;
	page.buttons[idx].id = `btn_${tr}_${tc}`;
	renderGrid();
	renderPreview();
}

function openButtonModal(row,col){
	ensurePages();
	editingButton = {row,col};
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const btn = (page.buttons||[]).find(b=>b.row===row && b.col===col);
	const defaults = {
		label:`Button ${row}${col}`,
		color: page.button_color || theme.button_color || theme.accent_color || '#ff9d2e',
		pressed_color: firstDefined(page.button_pressed_color, theme.button_pressed_color, '#ff7a1a'),
		border_color: page.button_border_color || theme.border_color || '#3a3a3a',
		border_width: page.button_border_width || 0,
		corner_radius: page.button_radius || theme.button_radius || 12,
		font_size: firstDefined(theme.button_font_size, 24),
		font_family: firstDefined(theme.button_font_family, 'montserrat'),
		text_align: 'center',
		momentary: false,
		can:{enabled:false,pgn:0,priority:6,source_address:0xF9,destination_address:0xFF,data:[0,0,0,0,0,0,0,0]}
	};
	const data = btn || defaults;
	document.getElementById('btn-label').value = data.label || '';
	document.getElementById('btn-color').value = data.color;
	document.getElementById('btn-pressed-color').value = firstDefined(data.pressed_color, defaults.pressed_color);
	document.getElementById('btn-border-color').value = firstDefined(data.border_color, defaults.border_color);
	document.getElementById('btn-border-width').value = firstDefined(data.border_width, defaults.border_width);
	document.getElementById('btn-corner-radius').value = firstDefined(data.corner_radius, defaults.corner_radius);
	document.getElementById('btn-font-size').value = data.font_size || 24;
	document.getElementById('btn-font-family').value = data.font_family || 'montserrat';
	document.getElementById('btn-text-align').value = data.text_align || 'center';
	document.getElementById('btn-momentary').checked = data.momentary || false;
	const canCfg = data.can || {};
	document.getElementById('btn-can-enabled').checked = canCfg.enabled || false;
	document.getElementById('btn-can-pgn').value = (canCfg.pgn || 0).toString(16).toUpperCase();
	document.getElementById('btn-can-priority').value = firstDefined(canCfg.priority, 6);
	document.getElementById('btn-can-src').value = (firstDefined(canCfg.source_address, 0xF9)).toString(16).toUpperCase();
	document.getElementById('btn-can-dest').value = (firstDefined(canCfg.destination_address, 0xFF)).toString(16).toUpperCase();
	const canData = (canCfg.data && canCfg.data.length) ? canCfg.data : defaults.can.data;
	document.getElementById('btn-can-data').value = canData.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	populateCanLibraryDropdown();
	toggleCanFields();
	document.getElementById('button-modal').classList.add('open');
}

function closeModal(){ document.getElementById('button-modal').classList.remove('open'); }

function saveButtonFromModal(){
	ensurePages();
	const page = config.pages[activePageIndex];
	if(!page.buttons) page.buttons = [];
	const idx = page.buttons.findIndex(b=>b.row===editingButton.row && b.col===editingButton.col);
	const canEnabled = document.getElementById('btn-can-enabled').checked;
	const canData = (document.getElementById('btn-can-data').value||'').split(' ').map(v=>parseInt(v,16)||0).slice(0,8);
	while(canData.length<8) canData.push(0);
	const button = {
		id:`btn_${editingButton.row}_${editingButton.col}`,
		row: editingButton.row,
		col: editingButton.col,
		row_span: 1,
		col_span: 1,
		label: document.getElementById('btn-label').value || 'Button',
		color: document.getElementById('btn-color').value,
		pressed_color: document.getElementById('btn-pressed-color').value,
		border_color: document.getElementById('btn-border-color').value,
		border_width: parseInt(document.getElementById('btn-border-width').value)||0,
		corner_radius: parseInt(document.getElementById('btn-corner-radius').value)||0,
		font_size: parseInt(document.getElementById('btn-font-size').value)||24,
		font_family: document.getElementById('btn-font-family').value,
		font_weight: '400',
		font_name: document.getElementById('btn-font-family').value+'_16',
		text_align: document.getElementById('btn-text-align').value,
		momentary: document.getElementById('btn-momentary').checked,
		can: {
			enabled: canEnabled,
			pgn: canEnabled ? parseInt(document.getElementById('btn-can-pgn').value,16)||0 : 0,
			priority: canEnabled ? parseInt(document.getElementById('btn-can-priority').value)||6 : 6,
			source_address: canEnabled ? parseInt(document.getElementById('btn-can-src').value,16)||0xF9 : 0xF9,
			destination_address: canEnabled ? parseInt(document.getElementById('btn-can-dest').value,16)||0xFF : 0xFF,
			data: canData
		}
	};
	if(idx>=0) page.buttons[idx] = button; else page.buttons.push(button);
	closeModal();
	renderGrid();
	renderPreview();
}

function deleteButtonFromModal(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.buttons = (page.buttons||[]).filter(b=>!(b.row===editingButton.row && b.col===editingButton.col));
	closeModal();
	renderGrid();
	renderPreview();
}

function toggleCanFields(){
	const show = document.getElementById('btn-can-enabled').checked;
	document.getElementById('can-config-wrapper').style.display = show ? 'grid' : 'none';
}

function renderPreview(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const headerCfg = config.header || {};
	const header = document.getElementById('preview-header');
	header.style.background = theme.surface_color || '#12141c';
	header.style.borderBottom = `${firstDefined(theme.header_border_width, 0)}px solid ${firstDefined(theme.header_border_color, theme.accent_color, '#ff9d2e')}`;
	document.getElementById('preview-logo').style.display = headerCfg.show_logo === false ? 'none' : 'block';
	document.getElementById('preview-title').textContent = headerCfg.title || 'CAN Control';
	document.getElementById('preview-subtitle').textContent = headerCfg.subtitle || '';
	document.getElementById('preview-title').style.color = theme.text_primary || '#fff';
	document.getElementById('preview-subtitle').style.color = theme.text_secondary || '#8d92a3';

	renderNav();

	const body = document.getElementById('preview-body');
	body.style.background = page.bg_color || theme.page_bg_color || '#0f0f0f';
	body.innerHTML = '';
	const grid = document.createElement('div');
	grid.className = 'preview-grid';
	grid.style.gridTemplateColumns = `repeat(${page.cols||2}, minmax(80px,1fr))`;
	for(let r=0;r<page.rows;r++){
		for(let c=0;c<page.cols;c++){
			const btn = (page.buttons||[]).find(b=>b.row===r && b.col===c);
			const el = document.createElement('div');
			el.className = 'preview-btn'+(btn ? '' : ' empty');
			const fill = firstDefined(btn && btn.color, page.button_color, theme.button_color, theme.accent_color, '#ff9d2e');
			el.style.background = fill;
			el.style.borderColor = firstDefined(btn && btn.border_color, page.button_border_color, theme.border_color, 'transparent');
			el.style.borderWidth = `${firstDefined(btn && btn.border_width, page.button_border_width, theme.border_width, 0)}px`;
			el.style.color = '#000';
			el.style.borderRadius = `${firstDefined(btn && btn.corner_radius, page.button_radius, theme.button_radius, 12)}px`;
			el.textContent = (btn && btn.label) || '+';
			el.dataset.row = r;
			el.dataset.col = c;
			el.draggable = !!btn;
			el.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', `${r},${c}`); };
			el.ondragover = (e)=>e.preventDefault();
			el.ondrop = (e)=>{
				e.preventDefault();
				const data = e.dataTransfer.getData('text/plain');
				if(!data) return;
				const [fr,fc] = data.split(',').map(n=>parseInt(n));
				if(isNaN(fr) || isNaN(fc)) return;
				moveButton(fr,fc,r,c);
			};
			el.onclick = ()=>openButtonModal(r,c);
			grid.appendChild(el);
		}
	}
	body.appendChild(grid);
}

function renderNav(){
	ensurePages();
	const nav = document.getElementById('preview-nav');
	const theme = config.theme || {};
	nav.innerHTML = '';
	nav.style.background = firstDefined(theme.surface_color, '#12141c');
	nav.style.borderBottom = `1px solid ${firstDefined(theme.border_color, '#20232f')}`;
	config.pages.forEach((p,idx)=>{
		const chip = document.createElement('div');
		chip.className = 'pill';
		chip.textContent = p.name || 'Page '+(idx+1);
		const active = idx===activePageIndex;
		chip.style.background = active ? (theme.nav_button_active_color || theme.accent_color || '#ff9d2e') : (p.nav_color || theme.nav_button_color || '#2a2a2a');
		chip.style.color = active ? '#16110a' : '#f2f4f8';
		chip.draggable = true;
		chip.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', idx); };
		chip.ondragover = (e)=>e.preventDefault();
		chip.ondrop = (e)=>{
			e.preventDefault();
			const from = parseInt(e.dataTransfer.getData('text/plain'));
			if(isNaN(from)||from===idx) return;
			const moved = config.pages.splice(from,1)[0];
			config.pages.splice(idx,0,moved);
			activePageIndex = idx;
			renderNav();
			renderPageList();
			renderPreview();
		};
		chip.onclick = ()=>setActivePage(idx);
		nav.appendChild(chip);
	});
}

function updateHeaderFromInputs(){
	config.header = config.header || {};
	config.header.title = document.getElementById('header-title-input').value || 'CAN Control';
	config.header.subtitle = document.getElementById('header-subtitle-input').value || '';
	config.header.title_font = document.getElementById('header-title-font').value || 'montserrat_24';
	config.header.subtitle_font = document.getElementById('header-subtitle-font').value || 'montserrat_12';
	config.header.show_logo = document.getElementById('header-show-logo').checked;
	renderPreview();
}

function hydrateThemeFields(){
	const theme = config.theme || {};
	document.getElementById('theme-surface').value = theme.surface_color || '#12141c';
	document.getElementById('theme-page-bg').value = theme.page_bg_color || '#0f0f0f';
	document.getElementById('theme-text-primary').value = theme.text_primary || '#f2f4f8';
	document.getElementById('theme-text-secondary').value = theme.text_secondary || '#8d92a3';
	document.getElementById('theme-border').value = theme.border_color || '#20232f';
	document.getElementById('theme-nav-button').value = theme.nav_button_color || '#2a2a2a';
	document.getElementById('theme-nav-active').value = theme.nav_button_active_color || '#ff9d2e';
	document.getElementById('theme-radius').value = theme.button_radius || 12;
	document.getElementById('theme-border-width').value = theme.border_width || 0;
	document.getElementById('theme-header-border').value = theme.header_border_color || '#ff9d2e';
	document.getElementById('theme-header-border-width').value = theme.header_border_width || 0;
}

function collectTheme(){
	const existing = config.theme || {};
	config.theme = {
		surface_color: document.getElementById('theme-surface').value,
		page_bg_color: document.getElementById('theme-page-bg').value,
		button_pressed_color: existing.button_pressed_color || '#ff7a1a',
		text_primary: document.getElementById('theme-text-primary').value,
		text_secondary: document.getElementById('theme-text-secondary').value,
		border_color: document.getElementById('theme-border').value,
		nav_button_color: document.getElementById('theme-nav-button').value,
		nav_button_active_color: document.getElementById('theme-nav-active').value,
		button_radius: parseInt(document.getElementById('theme-radius').value)||12,
		border_width: parseInt(document.getElementById('theme-border-width').value)||0,
		header_border_color: document.getElementById('theme-header-border').value,
		header_border_width: parseInt(document.getElementById('theme-header-border-width').value)||0,
		button_font_family: existing.button_font_family || 'montserrat',
		button_font_size: existing.button_font_size || 24
	};
}

function wireThemeInputs(){
	['theme-bg','theme-surface','theme-page-bg','theme-accent','theme-text-primary','theme-text-secondary','theme-border','theme-nav-button','theme-nav-active','theme-radius','theme-border-width','theme-header-border','theme-header-border-width'].forEach(id=>{
		const el = document.getElementById(id);
		if(!el) return;
		el.addEventListener('input', ()=>{ collectTheme(); renderPreview(); });
		el.addEventListener('change', ()=>{ collectTheme(); renderPreview(); });
	});
}

function updatePageConfig(){
	if(!config.theme) config.theme = {};
	config.theme.page_bg_color = document.getElementById('page-bg').value;
	config.theme.nav_button_active_color = document.getElementById('nav-active').value;
	config.theme.nav_button_color = document.getElementById('nav-inactive').value;
	config.theme.accent_color = document.getElementById('button-active').value;
	config.theme.button_pressed_color = document.getElementById('button-pressed').value;
	config.theme.button_radius = parseInt(document.getElementById('button-radius').value)||8;
	config.theme.border_width = parseInt(document.getElementById('button-border-width').value)||1;
	renderPreview();
}

function applyBaseline(){
	const pageBg = document.getElementById('baseline-page-bg').value;
	const navActive = document.getElementById('baseline-nav-active').value;
	const navInactive = document.getElementById('baseline-nav-inactive').value;
	const buttonActive = document.getElementById('baseline-button-active').value;
	const buttonPressed = document.getElementById('baseline-button-pressed').value;
	const buttonRadius = parseInt(document.getElementById('baseline-button-radius').value)||8;
	const buttonBorderWidth = parseInt(document.getElementById('baseline-button-border-width').value)||1;
	
	document.getElementById('page-bg').value = pageBg;
	document.getElementById('nav-active').value = navActive;
	document.getElementById('nav-inactive').value = navInactive;
	document.getElementById('button-active').value = buttonActive;
	document.getElementById('button-pressed').value = buttonPressed;
	document.getElementById('button-radius').value = buttonRadius;
	document.getElementById('button-border-width').value = buttonBorderWidth;
	
	updatePageConfig();
	showBanner('Baseline applied to page configuration','success');
}

function saveAsBaseline(){
	const pageBg = document.getElementById('page-bg').value;
	const navActive = document.getElementById('nav-active').value;
	const navInactive = document.getElementById('nav-inactive').value;
	const buttonActive = document.getElementById('button-active').value;
	const buttonPressed = document.getElementById('button-pressed').value;
	const buttonRadius = parseInt(document.getElementById('button-radius').value)||8;
	const buttonBorderWidth = parseInt(document.getElementById('button-border-width').value)||1;
	
	document.getElementById('baseline-page-bg').value = pageBg;
	document.getElementById('baseline-nav-active').value = navActive;
	document.getElementById('baseline-nav-inactive').value = navInactive;
	document.getElementById('baseline-button-active').value = buttonActive;
	document.getElementById('baseline-button-pressed').value = buttonPressed;
	document.getElementById('baseline-button-radius').value = buttonRadius;
	document.getElementById('baseline-button-border-width').value = buttonBorderWidth;
	
	if(!config.theme) config.theme = {};
	config.theme.baseline_page_bg = pageBg;
	config.theme.baseline_nav_active = navActive;
	config.theme.baseline_nav_inactive = navInactive;
	config.theme.baseline_button_active = buttonActive;
	config.theme.baseline_button_pressed = buttonPressed;
	config.theme.baseline_button_radius = buttonRadius;
	config.theme.baseline_button_border_width = buttonBorderWidth;
	
	showBanner('Current page configuration saved as baseline','success');
}

function hydrateHeaderFields(){
	const header = config.header || {};
	document.getElementById('header-title-input').value = header.title || 'CAN Control';
	document.getElementById('header-subtitle-input').value = header.subtitle || '';
	document.getElementById('header-show-logo').checked = header.show_logo !== false;
}

function populateFontSelects(){
	const fonts = config.available_fonts || [];
	const titleSel = document.getElementById('header-title-font');
	const subSel = document.getElementById('header-subtitle-font');
	[titleSel, subSel].forEach(sel=>{ sel.innerHTML=''; });
	fonts.forEach(f=>{
		const opt = document.createElement('option');
		opt.value = f.name;
		opt.textContent = f.display_name || f.name;
		titleSel.appendChild(opt.cloneNode(true));
		subSel.appendChild(opt);
	});
	const headerCfg = config.header || {};
	titleSel.value = headerCfg.title_font || 'montserrat_24';
	subSel.value = headerCfg.subtitle_font || 'montserrat_12';
}

function renderCanLibrary(){
	const list = document.getElementById('can-library-list');
	const items = config.can_library || [];
	if(items.length===0){ list.innerHTML = '<div class="muted">No messages yet.</div>'; return; }
	list.innerHTML = '';
	items.forEach((msg,idx)=>{
		const card = document.createElement('div');
		card.className = 'can-card';
		card.innerHTML = `<div style="display:flex;justify-content:space-between;align-items:center;">
			<div><strong>${msg.name}</strong><div class="muted">PGN 0x${msg.pgn.toString(16).toUpperCase()}</div></div>
			<div class="row" style="flex:0 0 auto;">
				<button class="btn small" onclick="editCanMessage(${idx})">Edit</button>
				<button class="btn small danger" onclick="deleteCanMessage(${idx})">Delete</button>
			</div>
		</div>
		<div class="muted" style="margin-top:6px;">${msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ')}</div>`;
		list.appendChild(card);
	});
}

function addCanMessage(){
	const name = prompt('Message name','New Message');
	if(!name) return;
	const pgn = parseInt(prompt('PGN (hex)','FEF6'),16);
	if(isNaN(pgn)) return;
	if(!config.can_library) config.can_library = [];
	config.can_library.push({ id:'msg_'+Date.now(), name, pgn, priority:6, source_address:0xF9, destination_address:0xFF, data:[0,0,0,0,0,0,0,0], description:'' });
	renderCanLibrary();
}

function editCanMessage(idx){
	const msg = config.can_library[idx];
	if(!msg) return;
	const name = prompt('Message name', msg.name);
	if(!name) return;
	const pgn = parseInt(prompt('PGN (hex)', msg.pgn.toString(16)),16);
	if(isNaN(pgn)) return;
	const dataStr = prompt('8 bytes (space separated)', msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' '));
	const data = dataStr.split(' ').map(b=>parseInt(b,16)||0).slice(0,8);
	while(data.length<8) data.push(0);
	config.can_library[idx] = {...msg, name, pgn, data};
	renderCanLibrary();
}

function deleteCanMessage(idx){
	if(!confirm('Delete this message?')) return;
	config.can_library.splice(idx,1);
	renderCanLibrary();
}

function importCanMessage(type){
	const templates = {
		windows:{name:'Windows',pgn:0xFEF6,data:[255,255,255,255,255,255,255,255]},
		locks:{name:'Locks',pgn:0xFECA,data:[0,0,0,0,255,255,255,255]},
		boards:{name:'Running Boards',pgn:0xFE00,data:[1,0,0,0,255,255,255,255]}
	};
	const t = templates[type];
	if(!t) return;
	if(!config.can_library) config.can_library = [];
	config.can_library.push({ id:'msg_'+Date.now(), name:t.name, pgn:t.pgn, priority:6, source_address:0xF9, destination_address:0xFF, data:t.data, description:'Quick import' });
	renderCanLibrary();
}

function populateCanLibraryDropdown(){
	const sel = document.getElementById('btn-can-library-select');
	if(!sel) return;
	sel.innerHTML = '<option value="">-- Select --</option>';
	(config.can_library||[]).forEach((msg,idx)=>{
		const opt = document.createElement('option');
		opt.value = idx;
		opt.textContent = `${msg.name} (PGN 0x${msg.pgn.toString(16).toUpperCase()})`;
		sel.appendChild(opt);
	});
}

function loadCanFromLibrary(){
	const sel = document.getElementById('btn-can-library-select');
	const idx = parseInt(sel.value);
	if(isNaN(idx)) return;
	const msg = config.can_library[idx];
	if(!msg) return;
	document.getElementById('btn-can-enabled').checked = true;
	document.getElementById('btn-can-pgn').value = msg.pgn.toString(16).toUpperCase();
	document.getElementById('btn-can-priority').value = msg.priority;
	document.getElementById('btn-can-src').value = msg.source_address.toString(16).toUpperCase();
	document.getElementById('btn-can-dest').value = msg.destination_address.toString(16).toUpperCase();
	document.getElementById('btn-can-data').value = msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	toggleCanFields();
}

async function scanWiFi(){
	const btn = document.getElementById('scan-btn');
	btn.textContent = 'Scanning...';
	btn.disabled = true;
	try{
		const res = await fetch('/api/wifi/scan');
		const data = await res.json();
		wifiNetworks = data.networks || [];
		renderWifiList();
		showBanner(`Found ${wifiNetworks.length} networks`,`success`);
	}catch(err){ showBanner('Scan failed: '+err.message,'error'); }
	btn.textContent = 'Scan';
	btn.disabled = false;
}

function renderWifiList(){
	const list = document.getElementById('wifi-results');
	if(!wifiNetworks.length){ list.innerHTML = '<div class="muted">No networks yet.</div>'; return; }
	list.innerHTML = '';
	wifiNetworks.forEach((net,idx)=>{
		const item = document.createElement('div');
		item.className = 'wifi-item';
		item.innerHTML = `<div><strong>${net.ssid}</strong><div class="muted">Channel ${net.channel||'?'} · RSSI ${net.rssi||''}</div></div><div>${net.secure?'Locked':'Open'}</div>`;
		item.onclick = ()=>{
			document.getElementById('sta-ssid').value = net.ssid;
			document.getElementById('sta-password').focus();
			document.querySelectorAll('.wifi-item').forEach(el=>el.classList.remove('active'));
			item.classList.add('active');
		};
		list.appendChild(item);
	});
}

function handleLogoUpload(evt){
	const file = evt.target.files[0];
	if(!file) return;
	if(file.size>512000){ showBanner('Logo too large (500KB max)','error'); evt.target.value=''; return; }
	const reader = new FileReader();
	reader.onload = (e)=>{
		if(!config.header) config.header={};
		config.header.logo_base64 = e.target.result;
		document.getElementById('logo-preview-img').src = e.target.result;
		document.getElementById('logo-preview').style.display='block';
		renderPreview();
	};
	reader.readAsDataURL(file);
}

function handleSleepIconUpload(evt){
	const file = evt.target.files[0];
	if(!file) return;
	if(file.size>512000){ showBanner('Sleep icon too large (500KB)','error'); evt.target.value=''; return; }
	const reader = new FileReader();
	reader.onload = (e)=>{
		if(!config.display) config.display={};
		config.display.sleep_icon_base64 = e.target.result;
		document.getElementById('sleep-icon-preview-img').src = e.target.result;
		document.getElementById('sleep-icon-preview').style.display='block';
	};
	reader.readAsDataURL(file);
}

function clearCustomLogo(){ if(!config.header) config.header={}; config.header.logo_base64=''; document.getElementById('logo-preview').style.display='none'; renderPreview(); }
function clearSleepIcon(){ if(!config.display) config.display={}; config.display.sleep_icon_base64=''; document.getElementById('sleep-icon-preview').style.display='none'; }

function hydrateDisplay(){
	const display = config.display || {};
	document.getElementById('display-brightness').value = display.brightness ?? 100;
	document.getElementById('brightness-value').textContent = `${display.brightness ?? 100}%`;
	document.getElementById('sleep-enabled').checked = display.sleep_enabled || false;
	document.getElementById('sleep-timeout').value = display.sleep_timeout_seconds ?? 60;
	if(display.sleep_icon_base64){
		document.getElementById('sleep-icon-preview').style.display='block';
		document.getElementById('sleep-icon-preview-img').src = display.sleep_icon_base64;
	}
}

async function loadConfig(){
	try{
		const res = await fetch('/api/config');
		config = await res.json();
		ensurePages();
		hydrateThemeFields();
		hydrateHeaderFields();
		populateFontSelects();
		hydratePageFields();
		hydrateDisplay();
		const wifiCfg = config.wifi || {};
		const apCfg = wifiCfg.ap || {};
		const staCfg = wifiCfg.sta || {};
		document.getElementById('ap-ssid').value = apCfg.ssid || 'CAN-Control';
		document.getElementById('ap-password').value = apCfg.password || '';
		document.getElementById('ap-enabled').checked = apCfg.enabled !== false;
		document.getElementById('sta-ssid').value = staCfg.ssid || '';
		document.getElementById('sta-password').value = staCfg.password || '';
		document.getElementById('sta-enabled').checked = staCfg.enabled || false;
		renderPageList();
		renderGrid();
		renderPreview();
		renderCanLibrary();
		showBanner('Config loaded','success');
	}catch(err){ showBanner('Load failed: '+err.message,'error'); }
}

async function saveConfig(){
	ensurePages();
	collectTheme();
	config.header = config.header || {};
	config.header.title = document.getElementById('header-title-input').value || 'CAN Control';
	config.header.subtitle = document.getElementById('header-subtitle-input').value || '';
	config.header.title_font = document.getElementById('header-title-font').value;
	config.header.subtitle_font = document.getElementById('header-subtitle-font').value;
	config.header.show_logo = document.getElementById('header-show-logo').checked;

	config.wifi = {
		ap:{ enabled: document.getElementById('ap-enabled').checked, ssid: document.getElementById('ap-ssid').value, password: document.getElementById('ap-password').value },
		sta:{ enabled: document.getElementById('sta-enabled').checked, ssid: document.getElementById('sta-ssid').value, password: document.getElementById('sta-password').value }
	};

	const existingDisplay = config.display || {};
	config.display = {
		brightness: parseInt(document.getElementById('display-brightness').value)||100,
		sleep_enabled: document.getElementById('sleep-enabled').checked,
		sleep_timeout_seconds: parseInt(document.getElementById('sleep-timeout').value)||60,
		sleep_icon_base64: existingDisplay.sleep_icon_base64 || ''
	};

	try{
		const res = await fetch('/api/config',{ method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(config) });
		if(!res.ok){ const text = await res.text(); throw new Error(text); }
		showBanner('Configuration saved. Reboot device to apply display changes.','success');
	}catch(err){ showBanner('Save failed: '+err.message,'error'); }
}

document.addEventListener('DOMContentLoaded',()=>{
	loadConfig();
	document.getElementById('logo-upload').addEventListener('change', handleLogoUpload);
	document.getElementById('sleep-icon-upload').addEventListener('change', handleSleepIconUpload);
	wireThemeInputs();
});
</script>
</body>
</html>
)rawliteral";
