const mainStatusKey="statusX";
// const statusMotherStore=new JsonProperty(mainStatusKey)
// const statusCtrl=new StatusControl(mainStatusKey,statusMotherStore);

const bookSheetRW=new SheetReaderWriter(SpreadsheetApp.getActive().getSheetByName("シート1"),1,0);
const bookSheetArray=new SheetArray(bookSheetRW);
const statusManagerBase=new StatusControlManager(bookSheetArray);
const statusManager=new StatusControlManagerWithNfc(statusManagerBase);


function onEdit(e){
  //https://coporilife.com/392/
  const sheetName = e.source.getSheetName();
  //編集されたシート名と対象にしたいシート名が一致したら実行
  if(sheetName === 'シート1'){
    //編集されたセルの行数を取得
    const row = e.range.getRow();
    //編集されたセルの列数を取得
    const col = e.range.getColumn();

    if(row === 1 && col === 1){
      /**
       * A1セルに変更があったらその値をstatusに反映する
       * statusCtrlやLongPollingをテストする目的の機能
       */
      const sheet = e.source.getActiveSheet();
      const valCell=sheet.getRange(1,1);
      const contStatus=Number(valCell.getValue());
      Logger.log(contStatus);
      statusManager.refStatusControl(mainStatusKey).updateStatus({"cont":contStatus});
    }
  }
}

//認証方法 -> 時間がたつとOAuthTokenが変わるのでこの方法は不完全？
//https://www.ka-net.org/blog/?p=12258
//CORSのエラー
//https://stackoverflow.com/questions/53433938/how-do-i-allow-a-cors-requests-in-my-google-script
//https://detail.chiebukuro.yahoo.co.jp/qa/question_detail/q13276711733

function doGet(e) {
  const type=e.parameter.type;
  Logger.log("detect get");
  switch(type){
    /**
     * ウェブアプリのhtmlを返す
     * response ウェブアプリのhtml
     */
    default:
    case null:{
      const ret=HtmlService.createTemplateFromFile('index');
      
      ret.mainStatus=JSON.stringify(statusManager.refStatusControl(mainStatusKey,true).status);
      if(e.parameter.key==null){
        ret.targetStatus="null";
        ret.currentKey="nfc_1";
      }else{
        ret.targetStatus=JSON.stringify(statusManager.refStatusControl(e.parameter.key).status);
        ret.currentKey=e.parameter.key;
      }
      const retOutput=ret.evaluate();
      retOutput.addMetaTag('viewport', 'width=device-width, initial-scale=1');
      return retOutput;
    }
    
  }
}

function doPost(e) {
  const type=e.parameter.type;
  Logger.log("detect post");
  Logger.log(`request type: ${type}`);
  let key=e.parameter.key;
  if(key==null){
    key=mainStatusKey;
  }
  switch(type){
    /**
     * Long Pollingを行う
     * postData {
     *  timeout_ms:タイムアウトするまでの時間,
     *  version:post元が所有するstatusのversion
     * }
     * response {
     *  ret:"change"|"timeout",
     *  status:現行のstatus
     * }
     */
    case "polling":{
      Logger.log(`cacheWAR start`);
      const postJson=JSON.parse(e.postData.getDataAsString());
      const targetStatusControl=statusManager.refStatusControl(key);
      const watcher=targetStatusControl.genWatcher(postJson.version);
      const timeoutTime=Date.now()+postJson.timeout;
      Logger.log(`cacheWAR receive postJson:${postJson}`);
      Logger.log(`request timeout:${postJson.timeout} version:${postJson.version}`);
      let retPayload=null;
      while (true){
        if(watcher.isChanged){
          retPayload={"ret":"change","status":targetStatusControl.status};
          break;
        }else if((Date.now())>timeoutTime){
          retPayload={"ret":"timeout","status":targetStatusControl.status};
          break;
        }
        Utilities.sleep(500);
      }
      const ret=ContentService.createTextOutput();
      ret.setContent(JSON.stringify(retPayload));
      ret.setMimeType(ContentService.MimeType.JSON);
      Logger.log(`cacheWAR ret response ${JSON.stringify(retPayload)} !!!!!`);
      return ret;
    }
    /** 
     * statusのupdateを行う
     * postData statusの差分
     * response 現行のstatus
     */
    case "updateStatus":{
      const jsonString = e.postData.getDataAsString();
      const targetStatusControl=statusManager.refStatusControl(key);
      const data = JSON.parse(jsonString);
      Logger.log(data);
      targetStatusControl.updateStatus(data);
      const ret=ContentService.createTextOutput();
      ret.setContent(JSON.stringify(targetStatusControl.status));
      ret.setMimeType(ContentService.MimeType.JSON);
      return ret;
    }

    case "showLed":{
      const jsonString = e.postData.getDataAsString();
      const targetStatusControl=statusManager.refStatusControl(mainStatusKey);
      const data = JSON.parse(jsonString);
      Logger.log(data);
      let ledCommandArray=[];
      if("pos" === data.key){
        ledCommandArray.push({
          num:data.val,
          color:0xFFFFFF
        });
      }else{
        const targetBookStore=bookSheetArray.getStoresBySearch(data.key,data.val);
        for(const book of targetBookStore){
          const storeData=book.data;
          if("pos" in storeData){
            let obj=new Object();
            obj.num=storeData.pos;
            if("colorCode" in storeData){
              obj.color=storeData.colorCode;
            }else{
              obj.color=0xFFFFFF;
            }
            ledCommandArray.push(obj);
          }
        }
      }
      
      let currentTapeLedVersion=0;
      if("tapeLed" in targetStatusControl.status){
        currentTapeLedVersion=targetStatusControl.status.tapeLed.version;
      }
      targetStatusControl.updateStatus({
        tapeLed:{
          version:currentTapeLedVersion+1,
          array:ledCommandArray
        }
      });
      const ret=ContentService.createTextOutput();
      ret.setContent(JSON.stringify(targetStatusControl.status));
      ret.setMimeType(ContentService.MimeType.JSON);
      return ret;
    }
  }
  
}

function myFunction() {
  // setJsonProperty("status",{"ver":1});
}