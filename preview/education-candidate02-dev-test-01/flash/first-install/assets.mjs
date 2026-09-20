export const IMAGES=Object.freeze([{"name": "boot", "url": "./firmware/bootloader.bin", "offset": 8192, "size": 21264, "sha256": "4f00f81aad82838f4e33555e322abfbfff7d1de947d339c86e50be820cbd7bb4"}, {"name": "table", "url": "./firmware/partition-table.bin", "offset": 32768, "size": 3072, "sha256": "d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb"}, {"name": "app", "url": "./firmware/entropylab.bin", "offset": 65536, "size": 1542288, "sha256": "42d5e3b40a3869157171552162189a281c84b893c8533fe3ae8266bfc4c47550"}].map(Object.freeze));
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
