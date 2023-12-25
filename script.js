import { randomString, randomIntBetween } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';
import ws from 'k6/ws';
import { check, sleep } from 'k6';

const sessionDuration = randomIntBetween(10000, 60000); // user session between 10s and 1m

export const options = {
  vus: 700,
  iterations: 700,
};

export default function () {
  const url = `ws://172.25.48.1:9010/echo`;
  const params = { tags: { my_tag: 'my ws session' } };

  const res = ws.connect(url, params, function (socket) {
    socket.on('open', function open() {
      console.log(`VU ${__VU}: connected`);

      socket.setInterval(function timeout() {
        socket.send(JSON.stringify({ event: 'SAY', message: `I'm saying ${randomString(5)}` }));
      }, randomIntBetween(2, 20)); // say something every 200-800milliseconds
    });

    socket.on('ping', function () {
      console.log('PING!');
    });

    socket.on('pong', function () {
      console.log('PONG!');
    });

    socket.on('close', function () {
      console.log(`VU ${__VU}: disconnected`);
    });

    socket.on('message', function (message) {
      console.log(`VU ${__VU} received: ${message}`);
    });

    socket.setTimeout(function () {
      console.log(`Closing the socket forcefully 3s after graceful LEAVE`);
      socket.close();
    }, sessionDuration + 3000);
  });

  console.log(url)
  console.log(res)
  check(res, { 'Connected successfully': (r) => r && r.status === 101 });
}