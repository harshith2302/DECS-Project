import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:8080';
const DURATION = __ENV.DURATION || '40s';


const GET_RATIO = __ENV.GET_RATIO ? parseFloat(__ENV.GET_RATIO) : 0.7;
const PUT_RATIO = __ENV.PUT_RATIO ? parseFloat(__ENV.PUT_RATIO) : 0.3;

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

export default function () {
  const keyspace = parseInt(__ENV.KEYSPACE || '10000');
  const key = Math.floor(Math.random() * keyspace) + 1;

  const r = Math.random();


  if (r < GET_RATIO) {
    http.get(`${BASE}/read?key=${key}`);


  } else {
    const value = `val-${__VU}-${Math.random().toString(36).slice(2, 8)}`;
    http.post(`${BASE}/create?key=${key}&value=${value}`);
  }
}
