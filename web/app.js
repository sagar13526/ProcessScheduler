(() => {
  const ALGOS = [
    { key: 'fcfs', name: 'First Come First Serve', note: 'Runs jobs in arrival order. A long job at the front makes everything behind it wait.' },
    { key: 'sjf', name: 'Shortest Job First', note: 'When the CPU frees up, runs the ready job with the shortest burst. Never interrupts a running job.' },
    { key: 'srtf', name: 'Shortest Remaining Time First', note: 'Like SJF, but a new job with less time left than the running one takes over the CPU.' },
    { key: 'priority', name: 'Priority (non-preemptive)', note: 'Runs the ready job with the lowest priority number. Never interrupts a running job.', usesPriority: true },
    { key: 'priority_p', name: 'Priority (preemptive)', note: 'A newly arrived job with a lower priority number takes the CPU immediately.', usesPriority: true },
    { key: 'rr', name: 'Round Robin', note: 'Each job runs for at most one quantum, then goes to the back of the queue.', usesQuantum: true },
  ];
  const COLORS = ['#2f5d8a', '#7a4e9c', '#2e7d6b', '#b4531f', '#8a6d1d', '#9c3d54', '#4a6b2f', '#4f5d75'];
  const SAMPLE = [[1, 0, 5, 2], [2, 1, 3, 1], [3, 2, 8, 3], [4, 3, 6, 2], [5, 4, 4, 4]];

  const $ = (id) => document.getElementById(id);
  const state = { procs: [], nextPid: 1, algo: 'fcfs', quantum: 2, tab: 'schedule', seq: 0 };

  const colorOf = (pid) => {
    const i = state.procs.findIndex((p) => p.pid === pid);
    return COLORS[(i < 0 ? 0 : i) % COLORS.length];
  };

  function setProcs(rows) {
    state.procs = rows.map(([pid, arrival, burst, priority]) => ({ pid, arrival, burst, priority }));
    state.nextPid = Math.max(0, ...state.procs.map((p) => p.pid)) + 1;
    renderProcs();
    schedule();
  }

  function renderProcs() {
    const body = $('proc-body');
    body.innerHTML = '';
    state.procs.forEach((p, idx) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `<td><span class="pid"><span class="dot" style="background:${COLORS[idx % COLORS.length]}"></span>P${p.pid}</span></td>` +
        ['arrival', 'burst', 'priority'].map((f) => `<td><input type="number" min="${f === 'burst' ? 1 : 0}" max="1000" step="1" data-f="${f}" aria-label="${f} time of P${p.pid}" value="${p[f]}"></td>`).join('') +
        `<td><button type="button" class="remove" aria-label="Remove P${p.pid}">&times;</button></td>`;
      tr.querySelectorAll('input').forEach((inp) => inp.addEventListener('input', () => {
        p[inp.dataset.f] = inp.value === '' ? NaN : Number(inp.value);
        schedule();
      }));
      tr.querySelector('.remove').addEventListener('click', () => {
        state.procs.splice(idx, 1);
        renderProcs();
        schedule();
      });
      body.appendChild(tr);
    });
  }

  function renderAlgos() {
    const list = $('algo-list');
    list.innerHTML = '';
    ALGOS.forEach((a) => {
      const b = document.createElement('button');
      b.type = 'button';
      b.setAttribute('role', 'radio');
      b.setAttribute('aria-checked', String(a.key === state.algo));
      b.textContent = a.name;
      b.addEventListener('click', () => { state.algo = a.key; renderAlgos(); schedule(); });
      list.appendChild(b);
    });
    const cur = ALGOS.find((a) => a.key === state.algo);
    $('algo-note').textContent = cur.note;
    $('quantum-row').hidden = !(cur.usesQuantum || state.tab === 'compare');
    document.querySelector('.proc-table').classList.toggle('no-prio', !cur.usesPriority && state.tab !== 'compare');
  }

  function problem() {
    if (!state.procs.length) return 'Add at least one process.';
    for (const p of state.procs) {
      if ([p.arrival, p.burst, p.priority].some((v) => !Number.isInteger(v) || v < 0 || v > 1000)) return `Use whole numbers from 0 to 1000 for P${p.pid}.`;
      if (p.burst < 1) return `Burst time of P${p.pid} must be at least 1.`;
    }
    if (!Number.isInteger(state.quantum) || state.quantum < 1 || state.quantum > 100) return 'Time quantum must be a whole number from 1 to 100.';
    return null;
  }

  function showMessage(text) { const m = $('message'); m.textContent = text || ''; m.hidden = !text; }

  let timer;
  function schedule() { clearTimeout(timer); timer = setTimeout(run, 200); }

  async function run() {
    const err = problem();
    if (err) { showMessage(err); return; }
    const mine = ++state.seq;
    try {
      const res = await fetch('/api/simulate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ algorithm: state.tab === 'compare' ? 'all' : state.algo, quantum: state.quantum, processes: state.procs }),
      });
      const data = await res.json();
      if (mine !== state.seq) return;  // a newer request superseded this one
      if (data.error) { showMessage(data.error); return; }
      showMessage('');
      state.tab === 'compare' ? renderCompare(data.results) : renderSchedule(data.results[0]);
    } catch (e) {
      showMessage("Can't reach the simulator. Start it with `python3 server.py` and open http://localhost:8000.");
    }
  }

  function renderSchedule(r) {
    $('gantt-title').textContent = `Gantt chart: ${r.name}`;
    const wrap = $('gantt');
    const total = r.endTime - r.startTime;
    const unit = Math.max(22, Math.min(60, Math.floor(($('gantt').parentElement.clientWidth - 8) / total)));
    const step = Math.max(1, Math.ceil(total / Math.floor(($('gantt').parentElement.clientWidth) / 34)));

    let blocks = '';
    r.gantt.forEach((s) => {
      const idle = s.pid === -1;
      const style = `left:${(s.start - r.startTime) * unit}px;width:${(s.end - s.start) * unit}px;` + (idle ? '' : `background:${colorOf(s.pid)}`);
      blocks += `<div class="block${idle ? ' idle' : ''}" style="${style}" title="${idle ? 'Idle' : 'P' + s.pid}: ${s.start} to ${s.end} (${s.end - s.start} units)">${idle ? 'idle' : 'P' + s.pid}</div>`;
    });
    let ticks = '';
    for (let t = 0; t <= total; t += step) ticks += `<span class="tick" style="left:${t * unit}px">${r.startTime + t}</span>`;
    if (total % step !== 0) ticks += `<span class="tick" style="left:${total * unit}px">${r.endTime}</span>`;

    wrap.style.setProperty('--unit', unit + 'px');
    wrap.style.width = total * unit + 'px';
    wrap.innerHTML = `<div class="track">${blocks}</div><div class="ruler">${ticks}</div>`;
    wrap.classList.remove('sweep'); void wrap.offsetWidth; wrap.classList.add('sweep');

    const m = r.metrics;
    const items = [['Avg waiting', m.avgWaiting], ['Avg turnaround', m.avgTurnaround], ['Avg response', m.avgResponse],
      ['CPU utilization', m.cpuUtilization + '%'], ['Throughput', m.throughput + ' / unit'], ['Context switches', m.contextSwitches]];
    $('summary').innerHTML = items.map(([k, v]) => `<div><dt>${k}</dt><dd>${v}</dd></div>`).join('');

    const head = ['Process', 'Arrival', 'Burst', 'Priority', 'Start', 'Completion', 'Waiting', 'Turnaround', 'Response'];
    $('metrics').innerHTML = `<thead><tr>${head.map((h) => `<th scope="col">${h}</th>`).join('')}</tr></thead><tbody>` +
      r.processes.map((p) => `<tr><td><span class="pid"><span class="dot" style="background:${colorOf(p.pid)}"></span>P${p.pid}</span></td><td>${p.arrival}</td><td>${p.burst}</td><td>${p.priority}</td><td>${p.start}</td><td>${p.completion}</td><td>${p.waiting}</td><td>${p.turnaround}</td><td>${p.response}</td></tr>`).join('') + '</tbody>';
  }

  function renderCompare(results) {
    const num = (r, k) => parseFloat(r.metrics[k]);
    const maxV = Math.max(...results.map((r) => Math.max(num(r, 'avgWaiting'), num(r, 'avgTurnaround'))), 1);
    const minW = Math.min(...results.map((r) => num(r, 'avgWaiting')));
    const minT = Math.min(...results.map((r) => num(r, 'avgTurnaround')));
    $('compare').innerHTML = results.map((r) => {
      const w = num(r, 'avgWaiting'), t = num(r, 'avgTurnaround');
      return `<div class="cmp-row"><div class="cmp-name">${r.name}<small>${r.metrics.contextSwitches} context switches, ${r.metrics.cpuUtilization}% CPU</small></div><div>` +
        `<div class="bar wait${w === minW ? ' best' : ''}"><span class="bar-label">Waiting</span><i style="width:${(w / maxV) * 100}%"></i><span>${r.metrics.avgWaiting}</span></div>` +
        `<div class="bar${t === minT ? ' best' : ''}"><span class="bar-label">Turnaround</span><i style="width:${(t / maxV) * 100}%"></i><span>${r.metrics.avgTurnaround}</span></div></div></div>`;
    }).join('');
  }

  function setTab(tab) {
    state.tab = tab;
    $('tab-schedule').setAttribute('aria-selected', String(tab === 'schedule'));
    $('tab-compare').setAttribute('aria-selected', String(tab === 'compare'));
    $('view-schedule').hidden = tab !== 'schedule';
    $('view-compare').hidden = tab !== 'compare';
    renderAlgos();
    schedule();
  }

  $('add-btn').addEventListener('click', () => {
    if (state.procs.length >= 30) return showMessage('The simulator supports up to 30 processes.');
    const last = state.procs[state.procs.length - 1];
    state.procs.push({ pid: state.nextPid++, arrival: last ? last.arrival + 1 : 0, burst: 4, priority: 1 });
    renderProcs();
    schedule();
  });
  $('sample-btn').addEventListener('click', () => setProcs(SAMPLE));
  $('random-btn').addEventListener('click', () => {
    const n = 4 + Math.floor(Math.random() * 3), rnd = (a, b) => a + Math.floor(Math.random() * (b - a + 1));
    setProcs(Array.from({ length: n }, (_, i) => [i + 1, rnd(0, 8), rnd(1, 9), rnd(1, 5)]));
  });
  $('quantum').addEventListener('input', (e) => { state.quantum = e.target.value === '' ? NaN : Number(e.target.value); schedule(); });
  $('tab-schedule').addEventListener('click', () => setTab('schedule'));
  $('tab-compare').addEventListener('click', () => setTab('compare'));
  window.addEventListener('resize', () => { clearTimeout(timer); timer = setTimeout(() => state.tab === 'schedule' && run(), 250); });

  renderAlgos();
  setProcs(SAMPLE);
})();
