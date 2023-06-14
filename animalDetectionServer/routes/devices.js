var express = require("express");
var router = express.Router();
var request = require('request');
const fs = require('fs');	// fs 모듈 선언
var path = require('path');
var db = require('../db');

router.get('/', (req, res) => {
	var sess = req.session;
	const title = "Sensor Control";
	var temp= "";
	//var obj = [];
	var temp= "";
	var ct= "";
	var key1 =""; 
	//var temps =[];
	var data = new Object();
	console.log("0!");
	if(sess.username) {	
		console.log("1!");
		
		request(options,function (err, resp, body) {
				//callback		  
			if(err) {
				console.log("error!");
			}	
			let obj = JSON.parse(body);				
			console.log(obj["m2m:cin"].con,obj["m2m:cin"].ct); 			
			data[0] = {con: obj["m2m:cin"].con, ct:obj["m2m:cin"].ct};
			console.log(__dirname); 			
			return res.render('./device_con.ejs',{title:title,
				is_logined : true,
				id : sess.username,
				data:data
			});      
			
		});
		//여기 호출 안됨     
		
	}else {
		sess.is_logined = false;
		sess.username = null;
        res.render('login',{
            is_logined : false
        });
		
	}
	
});
router.get('/in_temp', (req, res) => {
	var sess = req.session;
	const title = "Sensor View";
	//var obj = [];
	var temp= "";
	var ct= "";
	var key1 =""; 
	//var temps =[];
	var data = new Object();
	let array_=[];
	var cnt = 0;
	var data2 = new Object();
	var data3 = new Object();
	var data4 = new Object();
	var data5 = new Object();
	if(sess.username) {	
		//db에 저장된 AE의 정보 읽어 옮	
		var aeInfo = [
			{ idx:1,aeId:'DETECTION_AE1',cntInfo:['temp','uwb']},
			{ idx:2,aeId: 'DETECTION_AE2', cntInfo:['temp','uwb']},			
			{ idx:3,aeId: 'CAMERA_AE3', cntInfo:['cam','motor']},
			
		];	
					
		var img_temp = "../../images/temp_check.png";
		var img_uwb = "../../images/uwb_sensor.png";
		
		return res.render('device_view',{title:title,			
				is_logined : true,
				id : sess.username,
				aeInfo :aeInfo,
				img_temp :img_temp,
				img_uwb:img_uwb, 				
				
		});
     
		
	}else {
		sess.is_logined = false;
		sess.username = null;
        res.render('login',{
            is_logined : false
        });
		
	}
	
});
router.get('/out_control', (req, res) => {
	var sess = req.session;
	const title = "Sensor Control";
	var temp= "";
	//var obj = [];
	var temp= "";
	var ct= "";
	var key1 =""; 
	//var temps =[];
	var data = new Object();
	 
	//db에 저장된 AE의 정보 읽어 옮	
	var aeInfo = [
		{ idx:1,aeId:'DETECTION_AE1',cntInfo:['led','buzzer']},
		{ idx:2,aeId: 'DETECTION_AE2', cntInfo:['led','buzzer']},
		{ idx:3,aeId: 'CAMERA_AE3', cntInfo:['cam','motor']},
		//{ idx:4,aeId: 'cam1', cntInfo:['MOTOR']}
	];	
	var img_ledOn    = "../../images/warning_on.png";
	var img_ledOff   = "../../images/warning_off.png";	
	var img_soundOn  = "../../images/megaphone_on.png";
	var img_soundOff = "../../images/megaphone_off.png";	
	
	if(sess.username) {			
		
		 return res.render('device_con.ejs',{title:title,
			is_logined : true,
			id : sess.username,
			//data:data,
			aeInfo : aeInfo,
			img_ledOn :img_ledOn,
			img_ledOff :img_ledOff,
			img_soundOn :img_soundOn,
			img_soundOff :img_soundOff,			
		}); 
		   
		
	}else {
		sess.is_logined = false;
		sess.username = null;
        res.render('login',{
            is_logined : false
        });
		
	}
	
});
router.get('/cam_control', (req, res) => {
	var sess = req.session;
	const title = "Camera Control";
	var temp= "";
	//var obj = [];
	var temp= "";
	var ct= "";
	var key1 =""; 
	//var temps =[];
	var data = new Object();
	
	//db에 저장된 AE의 정보 읽어 옮
	
	var aeInfo = [
		{ idx:1,aeId:'CAMERA_AE3',cntInfo:['cam','motor']},
	]	
		
	var img_flashOn = "../../images/warning_on.png";
	var img_flashOff = "../../images/warning_off.png";
	var img_soundOn = "../../images/megaphone_on.png";
	var img_soundOff = "../../images/megaphone_off.png";
	var img_camera= "../../images/megaphone_off.png";	
	//var ae_info = 
	
	if(sess.username) {	
		//console.log("1!");
		
		request(options,function (err, resp, body) {
				//callback		  
			if(err) {
				console.log("error!");
			}	
			let obj = JSON.parse(body);				
			//console.log(obj["m2m:cin"].con,obj["m2m:cin"].ct); 			
			//data[0] = {con: obj["m2m:cin"].con, ct:obj["m2m:cin"].ct};
			data[0] = {con: 0, ct:0};
			//console.log(data.con,data.ct); 			
			return res.render('cam_con.ejs',{title:title,
				is_logined : true,
				id : sess.username,
				data:data,
		 		aeInfo : aeInfo,
				img_flashOn :img_flashOn,
				img_flashOff :img_flashOff,
				img_soundOn :img_soundOn,
				img_soundOff :img_soundOff,
				img_camera:img_camera 				
			});      
			
		});
		
	}else {
		sess.is_logined = false;
		sess.username = null;
        res.render('login',{
            is_logined : false
        });
		
	}
	
});
router.get('/in_list',(req,res,next)=>{
	var sess = req.session;	
	var file_list =[];	
	var user_id = user_id = sess.username;    	
	 	
	if(user_id) {	
		var folder = path.join(__dirname, 'public','uploads',user_id);
    	console.log('uploadPath :' + folder); 
		fs.readdir(folder, function(error, filelist){
			console.log(filelist); 
			file_list = filelist;
			file_list.forEach((file, index) => {
			  file_list[index] = file;
			  //console.log('f',file);			  
			})
			console.log("filelist");
			res.render('image_list',{is_logined : true,
				id : user_id,
				data :file_list 
			});

		});	
	}else {
		res.render('login',{is_logined : true,
			id : null,
		});	
	}

});

//컨테이너 con 값 최신 값 읽기
// async function ReadDevCin(aeId,cntName) {
// 	var aeId = aeId;
// 	var cntName= cntName; 
// 	var url = "";
// 	console.log('cnnName',cntName);
// 	if(cntName == "temp"){                   
// 		url = "http://192.168.1.11:7579/Mobius/"+ aeId +"/"+ cntName + "/la";                
// 	}	
// 	else if(cntName== "uwb" ){                    
// 		url = "http://192.168.1.11:7579/Mobius/"+ aeId +"/"+ cntName + "/la";                
// 	}              
	
// 	console.log(`aeId:${aeId},cntName:${cntName},uri:${url}`);                             
// 	var requestId = Math.floor(Math.random() * 10000);
// 	const data = await fetch(url, {
// 	   method: "get",
// 	   headers: {
// 				"Accept": "application/json",
// 				"X-M2M-Origin": aeId,
// 				"X-M2M-RI": requestId,
// 				"Content-Type": "application/json;charset=utf-8"
// 			 },                   
	   
// 	});
	
// 	const obj = await data.json();
// 	const obj1 = JSON.stringify(obj);                
// 	let obj2 = JSON.parse(obj1);
// 	//console.log(obj2["m2m:cin"].con,obj2["m2m:cin"].ct); //temp, ct 값 읽어옮
// 	//console.log(`obj1:${obj1}`); 

// 	return obj2;
	  
// } 
module.exports = router;