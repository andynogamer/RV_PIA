"""Temporary, local-file task queue for the currently open Unreal Editor.

Start explicitly from the editor Python console. No network listener, no startup hook.
Only workspace editor scripts can run. Stop with Invoke-Unreal.ps1 -Stop.
"""
import contextlib
import datetime
import io
import json
import os
from pathlib import Path
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
_rv_root = Path(unreal.Paths.project_dir()).resolve()
_rv_queue = _rv_root / 'Saved' / 'AgentBridge'
_rv_queue.mkdir(parents=True, exist_ok=True)
_rv_request = _rv_queue / 'request.json'
_rv_last = json.loads(_rv_request.read_text(encoding='utf-8-sig')).get('id') if _rv_request.exists() else None
_rv_check_time = 0.0


def _rv_tick(delta):
    global _rv_last, _rv_check_time
    if time.monotonic() - _rv_check_time < 0.25:
        return
    _rv_check_time = time.monotonic()
    if not _rv_request.exists():
        return
    try:
        request = json.loads(_rv_request.read_text(encoding='utf-8-sig'))
    except (PermissionError, FileNotFoundError):
        return
    task_id = request.get('id')
    if not task_id or task_id == _rv_last:
        return
    _rv_last = task_id
    output = {'id': task_id, 'ok': False}
    buffer = io.StringIO()
    try:
        if request.get('stop'):
            unreal.unregister_slate_post_tick_callback(_rv_handle)
            (_rv_queue / 'ready.json').unlink(missing_ok=True)
            output.update(ok=True, result='bridge stopped')
        else:
            script = (_rv_root / request['script']).resolve()
            allowed = (_rv_root / 'Scripts' / 'Editor', _rv_queue / 'tasks')
            if not any(script.is_relative_to(folder) for folder in allowed):
                raise ValueError('Task script must be inside workspace editor-task folders')
            scope = {'__name__': '__main__', '__file__': str(script),
                     'RV_ARGS': request.get('args', {}), 'RV_RESULT': None}
            with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
                exec(compile(script.read_text(encoding='utf-8-sig'), str(script), 'exec'), scope)
            output.update(ok=True, result=scope.get('RV_RESULT'))
    except Exception:
        output['error'] = traceback.format_exc()
    output['output'] = buffer.getvalue()
    output['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    target = _rv_queue / (task_id + '.json')
    temporary = target.with_suffix('.tmp')
    temporary.write_text(json.dumps(output, ensure_ascii=False, default=str), encoding='utf-8')
    temporary.replace(target)


_rv_handle = unreal.register_slate_post_tick_callback(_rv_tick)
(_rv_queue / 'ready.json').write_text(json.dumps({'ready': True, 'editor_pid': os.getpid(), 'root': str(_rv_root)}), encoding='utf-8')
unreal.log('RUTA: local editor task queue ready')
