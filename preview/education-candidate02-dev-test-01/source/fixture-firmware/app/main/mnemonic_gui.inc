#include "mnemonic_editor.inc"
static bool mn_hex(const char *s,size_t cap,size_t n){if(!memchr(s,0,cap)||strlen(s)!=n)return false;for(size_t i=0;i<n;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;return true;}
static bool mn_address(const char *s,size_t cap){if(!memchr(s,0,cap)||strlen(s)!=42||strncmp(s,"bc1q",4))return false;for(size_t i=4;i<42;i++)if(!strchr("qpzry9x8gf2tvdw0s3jn54khce6mua7l",s[i]))return false;return true;}
static void mn_result(const hex_result_t *r){
 if(!busy||!mn_pending||r->request_id!=mn_pending||r->revision!=mn_revision||r->mode!=MODE_MNEMONIC||r->words!=mn_target)return;
 busy=false;mn_pending=0;mn_refresh();
 if(r->rc!=0){const char *msg="Validation failed. Retry.";char b[80];if(r->rc==-1||r->rc==-2)msg="Validation unavailable: buffer contract error";else if(r->rc==-3)msg="Derivation failed. No result.";else if(r->rc==-6)msg="Checksum invalid. Check your words.";else if(r->rc==-4)msg="Noncanonical English text";else if(r->rc==-5)msg="Use exactly 12, 15, 18, 21 or 24 words";else if(r->rc<=-100&&r->rc>=-123){snprintf(b,sizeof b,"Unknown word at position %d",(int)(-r->rc-99));msg=b;}lv_label_set_text(mn_status,msg);return;}
 if(!mn_hex(r->entropy,sizeof r->entropy,mn_target*8/3)||!mn_hex(r->fingerprint,sizeof r->fingerprint,8)||!mn_address(r->address,sizeof r->address)){lv_label_set_text(mn_status,"Malformed result. Retry.");return;}
 strcpy(mn_recovered,r->entropy);lv_label_set_text(mn_fp,r->fingerprint);lv_label_set_text(mn_addr,r->address);mn_valid=true;lv_obj_remove_flag(mn_output,LV_OBJ_FLAG_HIDDEN);
}
