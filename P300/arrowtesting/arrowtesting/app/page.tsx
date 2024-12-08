import Image from "next/image";
import styles from "./page.module.css";
import { GoArrowDown, GoArrowDownLeft, GoArrowDownRight, GoArrowUpLeft,  GoArrowLeft, GoArrowRight, GoArrowUp, GoArrowUpRight } from "react-icons/go";
import { BsSignStopFill } from "react-icons/bs";
import { text } from "stream/consumers";


//let bcolorl45: any, bcolorl: any, bcolorbl45: any, bcolorr45: any, bcolorr: any, bcolorbr45: any, bcolorf: any, bcolorb: any, bcolors = "pink";
//let colorl45: any, colorl: any, colorbl45: any, colorr45: any, colorr: any, colorbr45: any, colorf: any, colorb: any, colors = "black";

  let sequence1 = ["pink", "pink", "pink", "pink", "pink", "pink", "pink", "pink", "pink"];
  let textcolours = ["black", "black", "black", "black", "black", "black", "black", "black", "black"];
  let sequence2: string[] = [];

  function pickSequence(array: (string | undefined)[]){
    let temp: any[] = [];
    array.forEach((element: any) => {
        temp.push(element);
    });
    //console.log("temp end: " + temp[temp.length - 1]);
    array.length = 0;
    let i =0;
    while (i < sequence1.length){
        let index = Math.floor(Math.random() * sequence1.length);
        if ((i == 0 && temp[temp.length - 1] == sequence1[index]) || array.includes(sequence1[index])){
            continue;
        } else {
            array.push(sequence1[index]);
            i++;
        }
    }
    //console.log("array: " + array[0]);
}
function flashSequence(array: any[]){
    //for(let j = 0; j < 3; j++){
        pickSequence(array);
    let i = 0;
    let interval = setInterval(function(){
        if (i < sequence1.length){
            array[i] = "pink";
            textcolours[i] = "black";
            setTimeout(function(){
                array[i] = "blue";
                textcolours[i] = "white";
            }, 100);
            i++;
        } else {
            clearInterval(interval);
        }
    }, 220);
}
function doTheThing(){
  //sequence1[0] = "blue";
   //flashSequence(_sequence1);
    //setTimeout(function(){
    for(let i = 0; i<7; i++){
    setTimeout(function(){flashSequence(sequence2);}, (2000*i));}//}, 2500);
    //flashSequence(_sequence3);
}
//}




export default function Home() {
 doTheThing();
  //bcolorl45 = "blue";
  return (
    <div className={styles.page}>
      <main className={styles.main}>
      
      <div className={styles.grid} id={styles.l45} style= {{backgroundColor: sequence1[0], color: textcolours[0]}}>
      <GoArrowUpLeft/>
      </div>
      <div className={styles.grid} id = {styles.fwd} style= {{backgroundColor: sequence1[1], color: textcolours[1]}}>
      <GoArrowUp/>
      </div>
      <div className={styles.grid} id = {styles.r45} style= {{backgroundColor: sequence1[2], color: textcolours[2]}}>
      <GoArrowUpRight/>
      </div>
      <div className={styles.grid} id = {styles.l} style= {{backgroundColor: sequence1[3], color: textcolours[3]}}>
      <GoArrowLeft />
      </div>
      <div className={styles.grid} id = {styles.stop} style= {{backgroundColor: sequence1[4], color: textcolours[4]}}>
      <BsSignStopFill />
      </div>
      <div className={styles.grid} id = {styles.r} style= {{backgroundColor: sequence1[5], color: textcolours[5]}}>
      <GoArrowRight />
      </div>
      <div className={styles.grid} id = {styles.bl45} style= {{backgroundColor: sequence1[6], color: textcolours[6]}}>
      <GoArrowDownLeft />
      </div>
      <div className={styles.grid} id = {styles.bkwd} style= {{backgroundColor: sequence1[7], color: textcolours[7]}}>
      <GoArrowDown />
      </div>
      <div className={styles.grid} id = {styles.br45} style= {{backgroundColor: sequence1[8], color: textcolours[8]}}>
      <GoArrowDownRight />
      </div>

      </main>
      
    </div>
  );
}

//function doTheThing(){
  //document.getElementById(styles.l45).style.backgroundColor = "pink";
  
//}