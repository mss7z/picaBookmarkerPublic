/**
 * StatusControlはこのシステム内で「Status」と呼ばれている状態を示すObjectを管理するためのものです。
 * JsonPropertyCacheを内部で使用します。
 */
class StatusControl{
  /**
   * StatusControlのインスタンスを作成します。
   * @param {string} keyName - PropertiesServiceの中で使用するkeyです。
   */
  constructor(keyName,motherStore){
    Logger.log(`StatusControl called key:${keyName} nowwowowowowowowow`);
    this._keyName=keyName;
    this._dataStore=motherStore;
    if(!this._dataStore.isExistData){
      this._dataStore.data={
        "version":1,
      };
      Logger.log(`data(key:${keyName}) reset`);
    }
    const mother=this;
    /**
     * this._WatcherはStatusの変更があるかを監視するクラスです。
     * 親であるStatusControlをmotherでキャプチャーします。
     */
    this._Watcher=class{
      /**
       * @param {number} checkedVersion - 使用主が現在所有するStatusのversion
       */
      constructor(checkedVersion){
        this._mother=mother;
        // this._checkedVersion=this._mother.version;
        Logger.log(`watcher constructor checkedVersion:${checkedVersion}`);
        this._checkedVersion=checkedVersion;
      }
      /**
       * コンストラクタで申告のあったcheckedVersionより新しい場合はtrueを返します。
       * @type {boolean}
       */
      get isChanged(){
        const acquiredVer=this._mother.version;
        const ret=(acquiredVer>this._checkedVersion);
        // this._checkedVersion=acquiredVer;
        return ret;
      }
    }
  }
  /**
   * statusの差分アップデートをします。
   * @param {Object} val - 変更のあったstatus 
   */
  updateStatus(val){
    const currentStatus=this.status;
    // ...はスプレッド構文
    const newStatus={
      ...currentStatus,
      ...val
    }
    return this.status=(newStatus);
  }
  /**
   * statusをセットすることでstatusを完全上書きします。
   * @type {Object} 
   */
  set status(val){
    // Logger.log("called");
    val.version=Math.trunc(this.version+1);
    // Logger.log(`in status val:${val["hel"]}`);
    this._dataStore.data=val;
    return val;
  }
  /**
   * 現行のstatus
   * @type {Object} 
   */
  get status(){
    return this._dataStore.data;
  }
  /**
   * @type {number} 現行のversion
   */
  get version(){
    return Math.trunc(this.status["version"]);
  }
  /**
   * 新しいWatcherインスタンスを作成します。
   * @return {this._Watcher} Watcherインスタンス
   */
  genWatcher(checkedVersion=0){
    return new this._Watcher(checkedVersion);
  }
};
function testStatus(){
  Logger.log(PropertiesService.getScriptProperties().getProperty("jax"));
  Logger.log("hellow");
  statusControl=new StatusControl("statusYtest");
  statusControl.status={"hel":"hello"};
  Logger.log(statusControl.status);
  const watcher=statusControl.genWatcher();
  Logger.log(`isChanged ${watcher.isChanged}`);
  Logger.log(`isChanged ${watcher.isChanged}`);
  Logger.log(`isChanged ${watcher.isChanged}`);
  statusControl.status={"hel":"hello"};
  Logger.log(`isChanged ${watcher.isChanged}`);
}
class ErrorStatusControl{
  constructor(keyName,msg){
    this.msg=`key:${keyName}に関するエラー\n${msg}`;
    this._Watcher=class{
      get isChanged(){
        return false;
      }
    }
  }
  get status(){
    return {
      version:0,
      errorMsg:this.msg
    }
  }
  genWatcher(_){
    return new this._Watcher();
  }
};

class StatusControlManager{
  constructor(bookSheetArray){
    this._statusControlDict=new Object();
    this._bookKeyPrefix="book_";
    this._searchKeyPrefix="search_";
    this._bookSheetArray=bookSheetArray;
  }
  refStatusControl(key,isNewIfNot=false){
    Logger.log(`StatusControlManager.refStatusControl key:${key}`);
    if(key in this._statusControlDict){
    }else{
      let motherStore;
      let index=null;
      if(key.startsWith(this._bookKeyPrefix)){
        index=parseInt(key.substring(this._bookKeyPrefix.length));
        if(isNaN(index))return new ErrorStatusControl(key,"インデックスが異常です");//null;
        Logger.log(`refStatusControl key:${key}->${index}`);
        motherStore=this._bookSheetArray.getStoreByIndex(index);
      }else if(key.startsWith(this._searchKeyPrefix)){
        const inputList=key.split("_");
        const searchKey=inputList[1];
        const searchVal=inputList[2];
        if(searchKey==null || searchVal==null){
          return new ErrorStatusControl(key,"search keyが異常です。");
        }else{
          const searchList=this._bookSheetArray.getStoresBySearch(searchKey,searchVal);
          if(searchList.length===0){
            return new ErrorStatusControl(key,"そのような本は今のところ見つかっていません");
          }else{
            motherStore=searchList[0];
          }
        }
      }else{
        motherStore=new JsonProperty(key);
      }
      const cacheStore=new JsonPropertyCache(key,motherStore);
      if(!isNewIfNot && !cacheStore.isExistData){
        return new ErrorStatusControl(key,"その本は現在存在しません");
      }
      this._statusControlDict[key]=new StatusControl(key,cacheStore);
      if(index!==null){
        if(this._statusControlDict[key].status.bookIndex==null){
          this._statusControlDict[key].updateStatus({
            bookIndex:index
          });
        }
      }
    }
    return this._statusControlDict[key];
  }
  genBookKeyFromIndex(index){
    return `${this._bookKeyPrefix}${index}`;
  }
  refBookStatusControlsBySearch(key,val,isNewIfNot=false){
    Logger.log(`refBookStatusControlBySearch key:${key},val:${val}`);
    const ret=[];
    const indexes=this._bookSheetArray.getIndexesBySearch(key,val);
    Logger.log(`search result indexes:[${indexes}]`);
    if(indexes.length===0){
      
      if(isNewIfNot){
        const newIndex=this._bookSheetArray.getLen();
        Logger.log("gen new book due to no exist key newIndex:"+newIndex);
        const stsCtrl=this.refStatusControl(this.genBookKeyFromIndex(newIndex),true);
        stsCtrl.updateStatus({
          [key]:val,
        });
        ret.push(stsCtrl);
      }
    }else{
      for(const index of indexes){
        Logger.log(`refBookStatusControlsBySearch for index:${index}`);
        ret.push(this.refStatusControl(this.genBookKeyFromIndex(index)));
      }
    }
    return ret;
  }
};

class NfcStatusControl{
  constructor(keyName,manager,statusKey,uidKeyInStatus){
    this._keyName=keyName;
    this._manager=manager;
    this._watchStatusControl=manager.refStatusControl(statusKey);
    this._uidKeyInStatus=uidKeyInStatus;
    this._selfStatusControl=manager.refStatusControl(this._keyName,true);
    // if(!this._selfStatusControl.isExistData){
    //   this._selfStatusControl.data={
    //     "version":1,
    //     "lastCheckedUid":"",
    //     "lastCheckedMotherVersion":0
    //   };
    //   Logger.log(`data(key:${keyName}) reset`);
    // }
    const mother=this;
    /**
     * this._WatcherはStatusの変更があるかを監視するクラスです。
     * 親であるStatusControlをmotherでキャプチャーします。
     */
    this._Watcher=class{
      /**
       * @param {number} checkedVersion - 使用主が現在所有するStatusのversion
       */
      constructor(checkedVersion){ //checkedversionはbookのもののほうが良い
        Logger.log(`watcher constructor checkedVersion:${checkedVersion}`);
        this._checkedVersion=checkedVersion
      }
      
      /**
       * コンストラクタで申告のあったcheckedVersionより新しい場合はtrueを返します。
       * @type {boolean}
       */
      get isChanged(){
        mother._checkVersion();
        return mother.version>this._checkedVersion;
      }
    }
  }
  _getUid(){
    return this._watchStatusControl.status[this._uidKeyInStatus];
  }
  _isHaveToUpdateVersion(){
    if(this._selfStatusControl.status.lastCheckedUid!=this._getUid()){
      return true;
    }
    if(this._selfStatusControl.status.lastCheckedMotherVersion<this._refTargetBookStatusControl().version){
      return true;
    }
    return false;
  }
  _checkVersion(){
    if(this._isHaveToUpdateVersion()){
      this._selfStatusControl.status={
        lastCheckedUid:this._getUid(),
        lastCheckedMotherVersion:this._refTargetBookStatusControl().version
      }
    }
  }
  _refTargetBookStatusControl(){
    const statusContorlData=this._watchStatusControl.status;
    //uidKeyなしの時の動作
    const retList=this._manager.refBookStatusControlsBySearch("uid",statusContorlData[this._uidKeyInStatus],true);
    return retList[0];
  }
  /**
   * statusの差分アップデートをします。
   * @param {Object} val - 変更のあったstatus 
   */
  updateStatus(val){
    const currentStatus=this.status;
    // ...はスプレッド構文
    const newStatus={
      ...currentStatus,
      ...val
    }
    return this.status=(newStatus);
  }
  /**
   * statusをセットすることでstatusを完全上書きします。
   * @type {Object} 
   */
  set status(val){
    this._refTargetBookStatusControl().status=val;
    return val;
  }
  /**
   * 現行のstatus
   * @type {Object} 
   */
  get status(){
    return {
      ...this._refTargetBookStatusControl().status,
      version:this.version
    };
  }
  /**
   * @type {number} 現行のversion
   */
  get version(){
    return Math.trunc(this._selfStatusControl.status.version);
  }
  /**
   * 新しいWatcherインスタンスを作成します。
   * @return {this._Watcher} Watcherインスタンス
   */
  genWatcher(checkedVersion=0){
    return new this._Watcher(checkedVersion);
  }
};

class StatusControlManagerWithNfc{
  constructor(motherManager){
    this._motherManager=motherManager;
    this._nfcKeyPrefix="nfc_";
  }
  refStatusControl(key,isNewIfNot){
    if(key.startsWith(this._nfcKeyPrefix)){
      //nfcの場合はisNewIfNotに関係なく新規作成が許可される
      const num=parseInt(key.substring(this._nfcKeyPrefix.length));
      if(isNaN(num))return null;
      return new NfcStatusControl(key,this._motherManager,"statusX",key+"_uid");
    }else{
      return this._motherManager.refStatusControl(key,isNewIfNot);
    }
  }
};