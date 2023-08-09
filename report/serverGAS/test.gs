  const selfUrl="https://script.google.com/macros/s/AKfycbxYOsqIE53pdwX4kid8On6YY0ewOdvuAXlyGdpfgDiVHSoVjso-IgCAOKTeIFzw1zX9/exec";
  const genLinkHTML=(key,inner)=>{
    // const queryParams=new URLSearchParams({
    //   key:key,
    // })
    return `<a href="${selfUrl}?key=${key}">${inner}</a>`;
  }
  const convMainTextWithLink=(str)=>{
    let firstIndex=0;
    const startStr=" [";
    const endStr="] ";
    let retStr="";
    while(true){
      const startIndexStart=str.indexOf(startStr,firstIndex);
      if(startIndexStart===-1){
        break;
      }
      const startIndexEnd=startIndexStart+startStr.length;
      const endIndexStart=str.indexOf(endStr,startIndexEnd);
      if(endIndexStart===-1){
        break;
      }
      const endIndexEnd=endIndexStart+endStr.length;

      const currentSubString=str.substring(startIndexEnd,endIndexStart);
      const currentLinkInner=str.substring(startIndexStart,endIndexEnd);
      let currentLinkHTML="";
      if(currentSubString.indexOf("=")===-1){
        // 含まれていない、標準キーの時
        currentLinkHTML=genLinkHTML(currentSubString,currentLinkInner);
      }else{
        const key=`search_${currentSubString.replace("=","_")}`;
        currentLinkHTML=genLinkHTML(key,currentLinkInner);
      }

      retStr+=str.substring(firstIndex,startIndexStart)+currentLinkHTML;
      firstIndex=endIndexEnd;
    }
    retStr+=str.substring(firstIndex);
    return retStr;
  }

function myFunction() {
  Logger.log(convMainTextWithLink("asdfsdf [helll] asdffffffff [uid=1234132]  asddd"));
}
