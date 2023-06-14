var express = require('express');
const crypto = require('crypto');
var flash = require('connect-flash');
const moment = require("moment");//시간 값 가져오기
const date = moment();
var fs = require('fs');
var path = require('path');
var router = express.Router();
var db = require('../db');
const cors = require("cors");
var sessionstorage = require('sessionstorage');
let corsOptions = {
    origin: 'http://localhost',
    credentials: true
}


/* GET users listing. */
router.get('/login',(req,res,next)=>{
	const sess = req.session;
	var fmsg = req.flash('flashMessages',`${sess.username}loged in`);
  	console.log(`flash:${fmsg}`);	
    if(sess.username){	
		req.flash('success',`${sess.username} loged in Success`);			
        res.render('index',{
            is_logined : true,
            id : sess.username,
			flashMessages:req.flash()
        });
    }else{
		sess.is_logined = false;
		sess.username = null;
		req.flash('error',`Failed to login`);
        res.render('login',{
            is_logined : false,
			flashMessages:req.flash()
        });
    }
});

router.post('/login', (req, res) => {
  
  const body = req.body;
  const id = body.id;
  const pw = body.pw;
  const sess = req.session;
  console.log('login!');
  //user의 비번이 일치할 경우를 위해 등록되는 ae 이름과 갯수 정보를 읽어 온다 db 테이블에 ae정보와 갯수 저장할 것
  //
  //
  db.get().query('select id,salt,password from users where id=?', [id], (err, data)=> {
    if(!err) {
		if(data.length > 0){ // 로그인 실패
			const salt = data[0].salt;
			const hashPassword = crypto.createHash("sha512").update(pw + salt).digest("hex");
			if(data[0].password == hashPassword) {
				//카톡 토큰 받기


				// 로그인 성공				
				// 쿠키 설정
				res.cookie('user',id,{expires:new Date(Date.now() + 900000),httpOnly:true});
				  // 세션에 추가
				sess.is_logined = true;
				sess.username = data[0].id;
				sess.email = data[0].email;
				sessionstorage.setItem('username',data[0].id);
				//req.session.pw = data.pw;					    
				sess.save(function(){ // 세션 스토어에 적용하는 작업
					console.log('session saved');
					//res.redirect('/authorize');
					res.render('index',{ // 정보전달
						id : sess.username,				
						is_logined : true,
						message: req.flash()
					});
					 
				});
			}
			else {
				sess.is_logined = false;
				console.log('password error');			
				res.render('register',{
					is_logined : false
				});			 
			}
		}
		else {
			console.log('you need to register');	
			sess.is_logined = false;
			res.render('register',{
				is_logined : false
			});	
		}
		  
	}
	else {
		if(data.length == 0){ // 로그인 실패
		  console.log('로그인 실패');	 
		  res.render('login',{
				is_logined : false
			});
		}
	}
  });

});

// 로그아웃
router.get('/logout',(req,res,next)=>{
  const sess = req.session;	
  console.log(sess.username);
  if(sess.username){		
    console.log('logout...');
    sess.is_logined = false;	
    res.clearCookie('sid');
    sess.destroy(function(err){
      if(err){
        console.log(err);
        console.log('로그아웃 실패');
      }
      else { 
        console.log('로그아웃 성공');		
        res.redirect('/');
      }
    });    

  } else {
    console.log('아직 로그인되어 있지 않음');
    res.redirect('/users/login');
  }

});
router.get('/in_list',(req,res,next)=>{
	var sess = req.session;	
	var file_list =[];	
	var user_id = sess.username;    	
	 	
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

module.exports = router;