from flask import Flask, request, Response
import uuid
import queue
import json
import os

app = Flask(__name__)

# Queue for incoming events
invocation_queue = queue.Queue()
# Dict to store responses keyed by request_id
responses = {}
# Dict to store response conditions to wait for result
response_events = {}

@app.route('/2018-06-01/runtime/invocation/next', methods=['GET'])
def invocation_next():
    """Endpoint for the custom runtime to get the next event."""
    try:
        # Wait for an event to be available
        event_data = invocation_queue.get(timeout=30)
        request_id = str(uuid.uuid4())
        
        headers = {
            'Lambda-Runtime-Aws-Request-Id': request_id,
            'Content-Type': 'application/json'
        }
        return Response(event_data, headers=headers)
    except queue.Empty:
        return Response(status=204)

@app.route('/2018-06-01/runtime/invocation/<request_id>/response', methods=['POST'])
def invocation_response(request_id):
    """Endpoint for the custom runtime to submit the result."""
    responses[request_id] = request.get_data()
    if request_id in response_events:
        response_events[request_id].put(True)
    return Response(status=202)

@app.route('/2015-03-31/functions/function/invocations', methods=['POST'])
def trigger_invocation():
    """Endpoint for the user/test to trigger a function call (mimicking RIE)."""
    event_data = request.get_data()
    request_id_queue = queue.Queue()
    
    # We need a way to track the specific request ID assigned by /next
    # For this simple emulator, we'll assume the next /next call picks this up.
    # To make it robust, we'd need a more complex mapping.
    # For testing one-at-a-time, this is fine.
    
    invocation_queue.put(event_data)
    
    # This is tricky because we don't know the request_id yet.
    # Let's simplify: the C runtime will call /next, get a UUID, then /response.
    # We'll just wait for the next available response in the responses dict.
    
    # Improved logic:
    # 1. Put event in queue
    # 2. Wait for ANY response to appear that wasn't there before
    # (Simple enough for single-threaded test)
    
    import time
    timeout = 10
    start_time = time.time()
    while time.time() - start_time < timeout:
        if responses:
            rid, res = responses.popitem()
            return Response(res, mimetype='application/json')
        time.sleep(0.1)
        
    return Response(json.dumps({"error": "Timeout waiting for runtime response"}), status=504)

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    print(f"Starting Lambda Runtime Emulator on port {port}...")
    app.run(host='0.0.0.0', port=port, threaded=True)
