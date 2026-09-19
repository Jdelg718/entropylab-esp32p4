const PUBLIC_IMAGES = Object.freeze([Object.freeze({name:'application',url:'./firmware/entropylab.bin',offset:0x10000,size:1539952,sha256:'a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b'})]);
export const IMAGES=Object.freeze([{"name": "boot", "url": "./firmware/bootloader.bin", "offset": 8192, "size": 21264, "sha256": "530330004af627bf2de7ac9b1d05ab34f8a655829a74005b96fc15dd7083f678"}, {"name": "table", "url": "./firmware/partition-table.bin", "offset": 32768, "size": 3072, "sha256": "d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb"}, {"name": "app", "url": "./firmware/entropylab.bin", "offset": 65536, "size": 1539952, "sha256": "a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b"}].map(Object.freeze));
export async function downloadImages(signal,mode='publicfirstinstall') {
 if(mode!=='publicfirstinstall')throw new Error('MODE_REFUSED');
 const assets=[];
 for(const image of IMAGES) {
  const url=new URL(image.url,import.meta.url);
  if(url.origin!==location.origin) throw new Error('origin');
  const response=await fetch(url,{signal,redirect:'error',credentials:'omit',cache:'no-store'});
  if(!response.ok || !response.body) throw new Error('asset unavailable');
  const declared=response.headers.get('content-length');
  if(declared!==null && Number(declared)!==image.size) throw new Error('size');
  const reader=response.body.getReader(); const bytes=new Uint8Array(image.size); let received=0;
  try { for(;;) { const {done,value}=await reader.read(); if(done)break;
   if(received+value.length>image.size)throw new Error('size'); bytes.set(value,received);received+=value.length;
  }} finally {await reader.cancel();}
  if(received!==image.size)throw new Error('size');
  const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
  if(hash!==image.sha256)throw new Error('hash');
  assets.push({offset:image.offset,bytes});
 }
 return assets;
}
