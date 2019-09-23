from pylxd import Client
from enum import Enum
from ws4py.client.threadedclient import WebSocketClient
import threading
import json

class EventType(Enum):
    All = 'all'
    Operation = 'operation'
    Logging = 'logging'
    Lifecycle = 'lifecycle'

client = Client()

class DummyClient(WebSocketClient):
    def closed(self, code, reason=None):
        print ("Closed down", code, reason)

    def received_message(self, m):
        print (m)
        json_msg = json.loads(m.data.decode('utf-8'))
        #print (json_msg['timestamp'])
        action = json_msg['metadata']['action'].split('-')[1]
        print (action)
        source = json_msg['metadata']['source'].split('/')[3]
        print(source)

try:
    ws_client = client.events(websocket_client=DummyClient, event_types=set([EventType.Lifecycle]))
    ws_client.connect()
    #wst = threading.Thread(target=ws_client.run_forever())
    #wst.daemon = True
    #wst.start()

    while True:
        pass
    
except KeyboardInterrupt:
    ws_client.close()