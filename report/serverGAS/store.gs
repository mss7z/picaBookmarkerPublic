/**
 * JsonPropertyはGASのPropertiesServiceをクラスにまとめたものです。
 * 情報の読み書きをObjectで行い、内部でJSONに変換して文字列にしてPropertiesServiceに保存します。
 * PropertiesServiceのドキュメント
 * https://developers.google.com/apps-script/reference/properties/properties-service?hl=ja
 */
class JsonProperty{
  constructor(keyName){
    this._keyName=keyName;
    this._proberty=PropertiesService.getScriptProperties();
  }
  get isExistData(){
    Logger.log(`cacheWAR JsonProperty isExistData was called :${this._keyName}`);
    return null!=this.rawDataStr;
  }
  set rawDataStr(val){
    // Logger.log(`hello called rawDataStr strval:${val}`);
    this._proberty.setProperty(this._keyName,val);
  }
  get rawDataStr(){
    Logger.log(`cacheWAR JsonProperty rawDataStr called warning key:${this._keyName}`);
    return this._proberty.getProperty(this._keyName);
  }
  set data(val){
    this.rawDataStr=JSON.stringify(val);
    // Logger.log(`after data rawDataStr:${this.rawDataStr} val:${val["hel"]}`);
  }
  get data(){
    return JSON.parse(this.rawDataStr); 
  }
};

function testJsonProperty(){
  const jp=new JsonProperty("testJsonProperty");
  Logger.log(`isExistData :${jp.isExistData}`);
  jp.data={"hel":"pro"};
  Logger.log(`append data`);
  Logger.log(`isExistData :${jp.isExistData}`);
  Logger.log(`rawdataStr :${jp.rawDataStr}`);
  Logger.log(`data[hel] :${jp.data["hel"]}`);
}

/**
 * JsonPropertyCacheはJsonPropertyにCache機能を追加したものです。
 * PropertiesServiceは読み書き回数に制限があります。
 * CacheServiceを用いることでPropertiesServiceの読み書き回数を削減することを目的としています。
 * 
 * CacheServiceのドキュメント
 * https://developers.google.com/apps-script/reference/cache?hl=ja
 * CacheServiceとPropertiesServiceの比較は次のサイトの概要が分かりやすいです。
 * https://qiita.com/golyat/items/ba5d9ce38ec3308d3757
 */
class JsonPropertyCache{
  constructor(keyName,motherStore){
    this._keyName=keyName;
    // if (motherStore===undefined){
    //   this._jsonProperty=new JsonProperty(this._keyName);
    // }else{
    //   this._jsonProperty=motherStore;
    // }
    this._jsonProperty=motherStore;
    
    this._cacheService=CacheService.getScriptCache();
    if(null==this._cacheService.get(this._keyName)){
      Logger.log(`cacheWAR call _loadFromJsonProperty from constructor`);
      this._loadFromJsonProperty();
    }
  }
  _loadFromJsonProperty(){
    Logger.log(`cacheWAR bec _loadFromJsonProperty was called`);
    if(this._jsonProperty.isExistData){
      this._cacheService.put(this._keyName,this._jsonProperty.rawDataStr,21600);
    }else{
      this._cacheService.remove(this._keyName);
    }
  }
  get isExistData(){
    const cacheRet=this._cacheService.get(this._keyName);
    if (cacheRet==null){
      Logger.log(`cacheWAR bec call _jsonProperty.isExistData from isExistData`);
      return this._jsonProperty.isExistData;
    }else{
      return true;
    }
  }
  set rawDataStr(val){
    this._cacheService.put(this._keyName,val,21600);
    this._jsonProperty.rawDataStr=val;
  }
  get rawDataStr(){
    // Logger.log(`JsonPropertyCACHE rawDataStr called ok`);
    const firstCacheRet=this._cacheService.get(this._keyName);
    if(firstCacheRet==null){
      Logger.log(`cacheWAR bec call _loadFromJsonProperty from rawDataStr`);
      this._loadFromJsonProperty();
      return this._cacheService.get(this._keyName);
    }else{
      return firstCacheRet;
    }
  }
  set data(val){
    this.rawDataStr=JSON.stringify(val);
  }
  get data(){
    return JSON.parse(this.rawDataStr); 
  }
}
function testJsonPropertyCache(){
  const jp=new JsonPropertyCache("testJsonProperty");
  Logger.log(`isExistData :${jp.isExistData}`);
  jp.data={"hel":"cache"};
  Logger.log(`append data`);
  Logger.log(`isExistData :${jp.isExistData}`);
  Logger.log(`rawdataStr :${jp.rawDataStr}`);
  Logger.log(`data[hel] :${jp.data["hel"]}`);
}


class SheetReaderWriter{
  constructor(targetSheet,lineOffset=0,posOffset=0){
    this._targetSheet=targetSheet;
    this._lineOffset=lineOffset;
    this._posOffset=posOffset;
    this._lock=LockService.getDocumentLock();
  }
  getRange(line,pos,lineLen,posLen){
    // if(lineLen===0 || posLen===0){
    //   return [[],];
    // }
    return this._targetSheet.getRange(
      line+this._lineOffset,pos+this._posOffset,
      lineLen,posLen
    );
  }
  getCell(line,pos){
    return this.getRange(line,pos,1,1);
  }
  
  enterLock(){
    while(true){
      if(this._lock.tryLock(10000)){
        return;
      }
    }
  }
  exitLock(){
    this._lock.releaseLock();
  }
  get uniqueKey(){
    return `ss_${this._targetSheet.getParent().getId()}_`+
      `s${this._targetSheet.getSheetId()}_${this._lineOffset}_${this._posOffset}`
  }
};
function testSheetReaderWriter(){
  const srw=new SheetReaderWriter(SpreadsheetApp.getActive().getSheetByName("シート1"),1,0);
  console.log("uniquekey:"+srw.uniqueKey);
}
class SheetArrayCore{
  constructor(sheetRW){
    this._sheetRW=sheetRW;
    this._controlKey="controlKeyOf"+this._sheetRW.uniqueKey;
    this._statusMotherData=new JsonProperty(this._controlKey);
    this._statusData=new JsonPropertyCache(this._controlKey,this._statusMotherData);
    if(!this._statusData.isExistData){
      this._statusData.data={
        len:0,
        searchCache:[]
      }
    }
  }
  _setLen(lenArg){
    this._statusData.data={
      ...this._statusData.data,
      len:lenArg
    };
  }
  _getSearchCache(key,val){
    if(this._statusData.data.searchCache==null){
      return null;
    }
    for(const entry of this._statusData.data.searchCache){
      if(entry.key===key,entry.val===val){
        return entry.result;
      }
    }
  }
  _setSearchCache(key,val,result){
    let lastData=this._statusData.data;
    if(lastData.searchCache==null){
      lastData.searchCache=[];
    }
    lastData.searchCache.push({
      key:key,
      val:val,
      result:result
    });
    this._statusData.data=lastData;
  }
  _clearSearchCache(){
    let lastData=this._statusData.data;
    lastData.searchCache=[];
    this._statusData.data=lastData;
  }
  getLen(){
    return this._statusData.data.len;
  }
  getRawByIndex(index){
    Logger.log(`getRawByIndex index:${index}`);
    return this._sheetRW.getCell(index+3,1).getValue();
  }
  _getIndexesBySearchInternal(key,val){
    if(this.getLen()===0){
      return [];
    }
    this._sheetRW.enterLock();
    const range=this._sheetRW.getRange(3,1,this.getLen(),1).getValues();
    console.log("getIndexesBySearch range:"+range);
    let ret=[];
    console.log("getIndexesBySearch len:"+this.getLen());
    for(let i=0;i<this.getLen();i++){
      const raw=range[i];
      const obj=JSON.parse(raw);
      if(obj[key]==val){
        ret.push(i);
      }
    }
    this._sheetRW.exitLock();
    return ret;
  }
  getIndexesBySearch(key,val){
    let ret=this._getSearchCache(key,val);
    if(ret!=null){
      return ret;
    }
    ret=this._getIndexesBySearchInternal(key,val);
    this._setSearchCache(key,val,ret);
    return ret;
  }
  setRawByIndex(index,val){
    this._sheetRW.enterLock();
    this._clearSearchCache();
    if(index>=this.getLen()){
      this._setLen(index+1);
    }
    const ret=this._sheetRW.getCell(index+3,1).setValue(val);
    this._sheetRW.exitLock();
    return ret;
  }
};
function testSheetArrayCore(){
  const srw=new SheetReaderWriter(SpreadsheetApp.getActive().getSheetByName("シート1"),1,0);
  const sac=new SheetArrayCore(srw);
  sac._clearSearchCache();
  // sac.setRawByIndex(0,JSON.stringify({"hel":"monster"}));
  // sac.setRawByIndex(1,JSON.stringify({"hel":"master"}));
  // sac.setRawByIndex(2,JSON.stringify({"hel":"monster"}));
  console.log(sac.getRawByIndex(0));
  console.log("cache search result:"+sac._getSearchCache("uid","B73BCAA6"));
  console.log("search result:"+sac.getIndexesBySearch("uid","B73BCAA6"));
  console.log("cache search result2:"+sac._getSearchCache("uid","B73BCAA6"));
}
class SheetArrayElement{
  constructor(core,index){
    this._core=core;
    this._index=index;
  }
  get isExistData(){
    Logger.log("book data "+(this._index)+" exist:"+(0<=this._index && this._index<this._core.getLen()));
    return 0<=this._index && this._index<this._core.getLen();
  }
  set rawDataStr(val){
    this._core.setRawByIndex(this._index,val);
  }
  get rawDataStr(){
    return this._core.getRawByIndex(this._index);
  }
  set data(val){
    this.rawDataStr=JSON.stringify(val);
  }
  get data(){
    return JSON.parse(this.rawDataStr); 
  }
};
class SheetArray{
  constructor(sheetRW){
    this._sheetRW=sheetRW;
    this._core=new SheetArrayCore(this._sheetRW);
  }
  // isExistIndex(index){
  //   return 0<=index && index<this._core.getLen();
  // }
  getStoreByIndex(index){
    return new SheetArrayElement(this._core,index);
  }
  getStoresBySearch(key,val){
    const indexes=this._core.getIndexesBySearch(key,val);
    let ret=[];
    for(const index of indexes){
      ret.push(this.getStoreByIndex(index));
    }
    return ret;
  }
  getIndexesBySearch(key,val){
    return this._core.getIndexesBySearch(key,val);
  }
  getLen(){
    return this._core.getLen();
  }
};
function testSheetArray(){
  const srw=new SheetReaderWriter(SpreadsheetApp.getActive().getSheetByName("シート1"),1,0);
  const sa=new SheetArray(srw);
  console.log("test get:"+sa.getStoreByIndex(0).rawDataStr);
}