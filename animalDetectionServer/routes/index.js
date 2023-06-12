var express = require('express');
var router = express.Router();
var request = require('request');
//var session = require('express-session');

///////////////Parameters/////////////////
var cseUrl = "http://192.168.1.11:7579";
//var cseUrl = "http://192.168.1.15:7579";

var aeName = "detection_server";
var aeId = "S"+aeName;
var aeIp = "192.168.1.15";
var cseName = "/Mobius";
var aePort = 3000;
var count = 0;
var sub_aeInfo = [
	{ aeId:'DETECTION_AE1',cntInfo:['led','uwb','temp']},
	{ aeId:'DETECTION_AE2',cntInfo:['led','uwb','temp']},
	{ aeId:'CAMERA_AE3',cntInfo:['cam','motor']}	
];	
arr_container = [];
arr_container[count] = {};
arr_container[count++] = cseName+"/"+sub_aeInfo[0].aeId+"/" + sub_aeInfo[0].cntInfo[0];
arr_container[count++] = cseName+"/"+sub_aeInfo[0].aeId+"/" + sub_aeInfo[0].cntInfo[1];
arr_container[count++] = cseName+"/"+sub_aeInfo[0].aeId+"/" + sub_aeInfo[0].cntInfo[2];
arr_container[count++] = cseName+"/"+sub_aeInfo[1].aeId+"/" + sub_aeInfo[1].cntInfo[0];
arr_container[count++] = cseName+"/"+sub_aeInfo[1].aeId+"/" + sub_aeInfo[1].cntInfo[1];
arr_container[count++] = cseName+"/"+sub_aeInfo[1].aeId+"/" + sub_aeInfo[1].cntInfo[2];
arr_container[count++] = cseName+"/"+sub_aeInfo[2].aeId+"/" + sub_aeInfo[2].cntInfo[0];
arr_container[count++] = cseName+"/"+sub_aeInfo[2].aeId+"/" + sub_aeInfo[2].cntInfo[1];

arr_actuator = [] ;
arr_actuator[0] = cseName + "/" + sub_aeInfo[0].aeId+"/" + "led";
arr_actuator[1] = cseName + "/" + sub_aeInfo[0].aeId+"/" + "buzzer";
arr_actuator[2] = cseName + "/" + sub_aeInfo[1].aeId+"/" + "led";
arr_actuator[3] = cseName + "/" + sub_aeInfo[1].aeId+"/" + "buzzer";
arr_actuator[4] = cseName + "/" + sub_aeInfo[2].aeId+"/" + "cam";
arr_actuator[5] = cseName + "/" + sub_aeInfo[2].aeId+"/" + "motor";

group_name = [];
group_name[0] = "grled";
group_name[1] = "grbuzzer";
//var group_path = "/Mobius/"+group_name+"/fopt";
arr_group = [] ;
arr_group[0] = cseName + "/" +"grled"+"/fopt";
arr_group[1] = cseName + "/" +"grbuzzer"+"/fopt";

var i = 0;
var detect_cnt = 0;
var detect_flag =0;

/* GET home page. */
//기본 페이지
router.get('/',(req,res)=>{
	
    const sess = req.session;
	console.log(req.cookies);
	if(sess.username!=null)
    	console.log(sess.username);    
	
    if(sess.username) {
        res.render('index',{
            is_logined : true,
            id: sess.username,
			pageTitle :"Home",				
        });
    }else{
        res.render('index',{
            is_logined : false,
			id: sess.username,
			pageTitle :"Home",					
        });
    }
});
router.post("/"+aeId, function (req, res) {	
	var req_body = req.body["m2m:sgn"].nev.rep["m2m:cin"];
	if(req_body != undefined) {
		console.log("\n[NOTIFICATION]")
		console.log(aeId);
		//console.log(req_body);
		var content = req_body.con;
		var ae = JSON.stringify(req_body.cr).split('S')[1].split('"')[0];
		const obj = JSON.stringify(content);
		//console.log(`Receieved : ${obj} from ${ae}`);		
		var value = JSON.parse(obj);
		//반환값이 '-1'이면 uwb 값으로 처리		
		if (value.indexOf(":") != -1) {
            if(ae=="DETECTION_AE1"){
                amb_temp1 = value.split(':')[0];
                obj_temp1 = value.split(':')[1];
                diff_temp1 = value.split(':')[2];
                console.log(`${ae} temperature:${amb_temp1},${obj_temp1},${diff_temp1}`); //temp, ct 값 읽어옮
            }
            if(ae=="DETECTION_AE2"){
                amb_temp2 = value.split(':')[0];
                obj_temp2 = value.split(':')[1];
                diff_temp2 = value.split(':')[2];
                console.log(`${ae} temperature:${amb_temp2},${obj_temp2},${diff_temp2}`); //temp, ct 값 읽어옮
            }
		} 
		else {
			if(ae=="DETECTION_AE1"){
                console.log(`${ae} detected:${value}`); //temp, ct 값 읽어옮
                if(value =="0") { //detect
                    detect_cnt +=1;
                    noti_flag1 = 0;
                } else if(value =="1") { //no detect
                    noti_flag1 = 1;
                    if(detect_cnt <= 0) detect_cnt= 0 ;
                    else detect_cnt -=1;
                }
            } else if(ae=="DETECTION_AE2"){
                console.log(`${ae} detected:${value}`); //temp, ct 값 읽어옮
                if(value =="0") { //detect
                    noti_flag2 = 0;
                    detect_cnt +=1;
                } else if(value =="1") { //no detect
                    noti_flag2 = 1;
                    if(detect_cnt <= 0) detect_cnt= 0 ;
                    else detect_cnt -=1;
                }
            } else if (ae=="CAMERA_AE3"){
				console.log(`${ae} detected:${value}`); //temp, ct 값 읽어옮
				if(value =="1")	{
					noti_cam = 1;	//camera send photo flag	
				} 
				// else if(value =="0") { //
				// 	noti_cam = 0;
				// }
			}
        }

        //if(detect_cnt>=5) {
        if(detect_cnt>=8) {
            
            console.log('send kakao message~~~~~~~~~~~~~~~~~~~~~~~~~~~~~',detect_cnt);
			//createContenInstance("grled","led","on",1);
			//createContenInstance("grbuzzer","buzzer","on",1);
            //createContenInstance("AE1FM0","buzzer","on",0);
            //createContenInstance("AE2FM0","buzzer","on",0);
            //send_message();
            detect_flag = 1;				
            if(detect_cnt > 8) detect_cnt = 0;
        }
        else {
            //createContenInstance("AE1FM0","buzzer","off",0);
            //createContenInstance("AE2FM0","buzzer","off",0);
            detect_flag = 0;
            
        }
        console.log('detect_cnt:',detect_cnt);
        console.log('noti:', detect_flag);
		console.log('cam:',noti_cam);
            
			
	}
	
	res.sendStatus(200);
	
});
router.get("/noti_data", (req, res) => {
    var resData = {'noti_flag1': noti_flag1,
                    'amb1':amb_temp1,'obj1':obj_temp1,'diff1':diff_temp1,
                    'noti_flag2': noti_flag2,
                    'amb2':amb_temp2,'obj2':obj_temp2,'diff2':diff_temp2,
                    'detect_flag':detect_flag,'noti_cam':noti_cam,
    };
	console.log("detect",resData.detect_flag);
    res.json(resData);
});

createAE();
//resetAE();
function createAE(){
	console.log("\n[REQUEST]");
	var method = "POST";
	var url= cseUrl+"/Mobius";
	var resourceType=2;
	var requestId = Math.floor(Math.random() * 10000);
	var representation = {
		"m2m:ae":{
			"rn":aeName,			
			"api":"app.company.com",
			"rr":"true",
			"poa":["http://"+aeIp+":"+aePort]
		}
	};

	console.log(method+" "+url);
	console.log(representation);

	var options = {
		url: url,
		method: method,
		headers: {
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
			"Content-Type": "application/json;ty="+resourceType
		},
		json: representation
	};

	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{
			console.log(response.statusCode);
			console.log(body);
			if(response.statusCode==409){ //충돌날 때 기존의 AE 제거함
				resetAE();
			}else{
				
				createSubscription("DETECTION_AE1","uwb");
				createSubscription("DETECTION_AE1","temp");			
				
				createSubscription("DETECTION_AE2","uwb");
				createSubscription("DETECTION_AE2","temp");
				
			    createSubscription("CAMERA_AE3","cam");//not working
				//group
				createGroup("grled");
				createGroup("grbuzzer");			
			}
		}
	});
}

function resetAE(){
	console.log("\n[REQUEST]");
	var method = "DELETE";
	var url= cseUrl+"/Mobius/"+aeName;
	var requestId = Math.floor(Math.random() * 10000);

	console.log(method+" "+url);

	var options = {
		url: url,
		method: method,
		headers: {
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
		}
	};

	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{			
			console.log(response.statusCode);
			console.log(body);
			createAE();
		}
	});
}
function resetSub(sub_aeId,cntName){	
	var idx = "";
	if(sub_aeId =="DETECTION_AE1") {
		if(cntName == "led"){			
			idx = 0;
		}
		else if(cntName == "uwb"){			
			idx = 1;
		}
		else if(cntName == "temp"){			
			idx = 2;
		}		
	}
	else if(sub_aeId =="DETECTION_AE2") {
		if(cntName == "led"){			
			idx = 3;
		}
		else if(cntName == "uwb"){
			idx = 4;
		}
		else if(cntName == "temp"){
			idx = 5;
		}		
	}
	else if(sub_aeId =="CAMERA_AE3") {
		if(cntName == "cam"){			
			idx = 6;
		}
		else if(cntName == "motor"){
			idx = 7;
		}			
	}

	
	console.log("\n[REQUEST]");
	var method = "DELETE";
	var requestId = Math.floor(Math.random() * 10000);		
	var url= cseUrl+arr_container[idx] +'/sub';

	var options = {
		url: url,
		method: method,
		headers: {
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
		}
	};

	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{
			console.log(response.statusCode);
			console.log(body);
			createSubscription(sub_aeId,cntName);
		}
	});
	
}

function createSubscription(sub_aeId,cntName){	
	var idx = "";	
	if(sub_aeId =="DETECTION_AE1") {
		if(cntName == "led"){			
			idx = 0;
		}
		else if(cntName == "uwb"){			
			idx = 1;
		}
		else if(cntName == "temp"){			
			idx = 2;
		}		
	}
	else if(sub_aeId =="DETECTION_AE2") {
		if(cntName == "led"){			
			idx = 3;
		}
		else if(cntName == "uwb"){
			idx = 4;
		}
		else if(cntName == "temp"){
			idx = 5;
		}		
	}
	else if(sub_aeId =="CAMERA_AE3") {
		if(cntName == "cam"){			
			idx = 6;
		}
		else if(cntName == "motor"){			
			idx = 7;
		}
		
	}

    
	console.log("\n[REQUEST]");
	var method = "POST";
	var url= cseUrl+arr_container[idx];
	var resourceType=23;
	var requestId = Math.floor(Math.random() * 10000);
	var representation = {
		"m2m:sub": {
			"rn": "sub",
			"nu": ["http://"+aeIp+":"+aePort+"/"+aeId+"?ct=json"],
			"nct": 2,
			"enc": {
				"net": [3]
			}
		}
	};

	console.log(method+" "+url);
	console.log(representation);

	var options = {
		url: url,
		method: method,
		headers:{
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
			"Content-Type": "application/json;ty="+resourceType
		},
		json: representation
	};
	console.log(options.headers)
	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{
			console.log(response.statusCode);
			console.log(body);
			if(response.statusCode==409){//충돌날 때 기존의 sub 제거함
				//resetSub(cntName);//2023.06.06
				console.log("[reset sub]");
			}
		}

	});
	
}

function createContenInstance(sub_aeId,cntName,value,group){
	var idx = "";
	var gidx = "";
	if(group==0){
		if(sub_aeId =="DETECTION_AE1"){
			if(cntName =="led"){
				idx = 0;
			}
			else if(cntName =="buzzer") {
				idx = 1;
			} 
		}
		if(sub_aeId =="DETECTION_AE2"){
			if(cntName =="led"){
				idx = 2;
			}
			else if(cntName =="buzzer") {
				idx = 3;
			} 
		}
	} else if(group==1){
		if(sub_aeId =="grled"){			
				gidx = 0;
		}
		else if(cntName =="grbuzzer") {
				gidx = 1;
		} 			

	}

	if(arr_actuator.length == 0){
		console.log("No more paths to create")
	}else{
		console.log("\n[REQUEST]");
		var method = "POST";
		if(group==0){
			var url= cseUrl+arr_actuator[idx];
		}
		else if(group==1){
			var url= cseUrl+arr_group[gidx];
		}
		var resourceType=4;
		var requestId = Math.floor(Math.random() * 10000);
		var representation = {
			"m2m:cin":{
					"con": value
				}
			};

		console.log(method+" "+url);
		console.log(representation);

		var options = {
			url: url,
			method: method,
			headers: {
				"Accept": "application/json",
				"X-M2M-Origin": aeId,
				"X-M2M-RI": requestId,
				"Content-Type": "application/json;ty="+resourceType
			},
			json: representation
		};

		request(options, function (error, response, body) {
			console.log("[RESPONSE]");
			if(error){
				console.log(error);
			}else{
				console.log(response.statusCode);
				console.log(body);
			}
		});
	}
}
function createGroup(grName){
	console.log("\n[REQUEST]");
	var gr_name ="";
	var cnt ="";
	var method = "POST";
	var url= cseUrl+"/Mobius";
	var resourceType=9;
	var requestId = Math.floor(Math.random() * 10000);
	if(grName =="grled"){		
		gr_name = "grled";//group_name[0];//"grled";
		cnt = "led";

	} else if(grName=="grbuzzer"){
		gr_name = "grbuzzer";//group_name[1];//"grbuzzer";
		cnt = "buzzer";
	}
	var representation = {
		"m2m:grp":{
			"rn":gr_name,
			"mid":[
				"Mobius/DETECTION_AE1/"+cnt,
				"Mobius/DETECTION_AE2/"+cnt,
				//"Mobius/AE3ESP0/"+cnt
			],
			"mnm":10
		}
	};

	console.log(method+" "+url);
	console.log(representation);

	var options = {
		url: url,
		method: method,
		headers: {
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
			"Content-Type": "application/json;ty="+resourceType
		},
		json: representation
	};

	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{
			console.log(response.statusCode);
			console.log(body);
			if(response.statusCode==409){
				resetGroup(gr_name);
				console.log(response.statusCode);
			}else{
				start_interval();

			}
		}
	});
}

function resetGroup(grName){
	var gr_name ="";
	if(grName =="grled"){		
		gr_name = "grled";//group_name[0];//"grled";

	} else if(grName=="grbuzzer"){
		gr_name = "grbuzzer";//group_name[1];//"grbuzzer";
	}
	console.log("\n[REQUEST]");
	var method = "DELETE";
	var url= cseUrl+"/Mobius/"+gr_name;
	var requestId = Math.floor(Math.random() * 10000);

	console.log(method+" "+url);	
	var options = {
		url: url,
		method: method,
		headers: {
			"Accept": "application/json",
			"X-M2M-Origin": aeId,
			"X-M2M-RI": requestId,
		}
	};
	request(options, function (error, response, body) {
		console.log("[RESPONSE]");
		if(error){
			console.log(error);
		}else{
			console.log(response.statusCode);
			console.log(body);
			createGroup(gr_name);
		}
	});
}

// function createContenInstance(path,value){
// 	console.log("\n[REQUEST]");
// 	var method = "POST";
// 	var url= cseUrl+path;
// 	var resourceType=4;
// 	var requestId = Math.floor(Math.random() * 10000);
// 	var representation = {
// 		"m2m:cin":{
// 				"con": value
// 			}
// 		};

// 	console.log(method+" "+url);
// 	console.log(representation);

// 	var options = {
// 		url: url,
// 		method: method,
// 		headers: {
// 			"Accept": "application/json",
// 			"X-M2M-Origin": aeId,
// 			"X-M2M-RI": requestId,
// 			"Content-Type": "application/json;ty="+resourceType
// 		},
// 		json: representation
// 	};

// 	request(options, function (error, response, body) {
// 		console.log("[RESPONSE]");
// 		if(error){
// 			console.log(error);
// 		}else{
// 			console.log(response.statusCode);
// 			console.log(body);
// 		}
// 	});
// }
function start_interval(){
	setInterval(function () {
		if(i==1){
			//createContenInstance(group_path,i);
			i=0;
		}
		else{
			//createContenInstance(group_path,i);
			i=1;
		}
	},3000);
}


module.exports = router;
