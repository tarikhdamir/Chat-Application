< !DOCTYPE html >
    <html>
        <head>
            <title>Chat Client</title>
        </head>
        <body>
            <h1>Chat Client</h1>
            <input id="room" type="text" placeholder="Room">
                <input id="message" type="text" placeholder="Message">
                    <button onclick="sendMessage()">Send</button>
                    <div id="messages"></div>

                    <script type="text/javascript">
                        var ws = new WebSocket("ws://localhost:9002");

                        ws.onopen = function() {
                            console.log("Connected to server");
        };

                        ws.onmessage = function(event) {
            var messagesDiv = document.getElementById("messages");
                        messagesDiv.innerHTML += "<p>" + event.data + "</p>";
                        console.log("Received message: " + event.data);
        };

                        ws.onclose = function() {
                            console.log("Disconnected from server");
        };

                        function sendMessage() {
            var room = document.getElementById("room").value;
                        var message = document.getElementById("message").value;
                        ws.send(room + ":" + message);
        }
                    </script>
                </body>
            </html>
