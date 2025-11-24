import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:8080';
const DURATION = __ENV.DURATION || '40s';

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

const HOT_KEY_MAX = 3000;

export default function () {
  const key = Math.floor(Math.random() * HOT_KEY_MAX) + 1;

  http.get(`${BASE}/read?key=${key}`);


}
