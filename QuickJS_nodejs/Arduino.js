'use strict';

const fetch = require('node-fetch');
const Headers = fetch.Headers;

class Arduino{
  constructor(base_url = ""){
    this.base_url = base_url;

    this.console = {
      log: this.console_log.bind(this)
    }
  }

  set_baseUrl(base_url){
    if( base_url.endsWith('/') )
      this.base_url = base_url.slice(0, -1);
    else
      this.base_url = base_url;
  }

  async millis(){
    return this.webapi_request("/millis", {});
  }

  async reboot(){
    return this.webapi_request("/reboot", {});
  }

  async pause(){
    return this.webapi_request("/pause", {});
  }

  async resume(){
    return this.webapi_request("/resume", {});
  }

  async restart(){
    return this.webapi_request("/restart", {});
  }

  async start(){
    return this.webapi_request("/start", {});
  }

  async stop(){
    return this.webapi_request("/stop", {});
  }

  async getStatus(){
    return this.webapi_request("/getStatus", {});
  }

  async setSyslogServer(host, port){
    return this.webapi_request("/setSyslogServer", { host: host, port: port });
  }

  async getSyslogServer(){
    return this.webapi_request("/getSyslogServer", {});
  }

  async code_upload(code, fname){
    if( fname )
      await this.webapi_request("/code-upload", { code: code, fname: fname } );
    else
      await this.webapi_request("/code-upload", { code: code } );
  }

  async code_upload_main(code, autoupdate){
    if( autoupdate !== undefined )
      await this.webapi_request("/code-upload", { code: code, autoupdate: autoupdate } );
    else
      await this.webapi_request("/code-upload", { code: code } );
  }

  async code_eval(code){
    await this.webapi_request("/code-eval", { code: code } );
  }
  
  async code_download(fname){
    if( fname )
      return this.webapi_request("/code-download", {fname: fname} );
    else
      return this.webapi_request("/code-download", {} );
  }

  async code_list(){
    return this.webapi_request("/code-list", {} );
  }
  
  async code_delete(fname){
    return this.webapi_request("/code-delete", { fname: fname} );
  }

  async console_log(msg){
    await this.webapi_request('/console-log', {msg: msg});
  }

  async getIpAddress(){
    return this.webapi_request("/getIpAddress", {} );
  }

  async getMacAddress(){
    return this.webapi_request("/getMacAddress", {} );
  }
  
  async getDeviceModel(){
    return this.webapi_request("/getDeviceModel", {} );
  }

  async customCall(message){
    return this.customcall_request( message );
  }

  async webapi_request(endpoint, body) {
    var params = {
      endpoint: endpoint,
      params: body
    };
    console.log(params);
    var json = await this.do_post(this.base_url + '/endpoint', params);
    console.log(json);
    if(json.status != "OK" )
      throw "status not OK";
    return json.result;
  }

  async customcall_request(message) {
    var params = {
      message: message
    };
    console.log(params);
    var json = await this.do_post(this.base_url + '/customcall_post', params);
    console.log(json);
    if(json.status != "OK" )
      throw "status not OK";
    return json.message;
  }

  async do_post(url, body) {
    const headers = new Headers({ "Content-Type": "application/json" });
  
    return fetch(url, {
      method: 'POST',
      body: JSON.stringify(body),
      headers: headers
    })
    .then((response) => {
      if (!response.ok)
        throw 'status is not 200';
      return response.json();
    });
  }
}

module.exports = Arduino;