var cargas=0.17, categoria, texto
  categorias = [
    //{"cat":0, "from":0,     "to":0,     "basis":0,    "perc":0.00},
    {"cat":1, "from":0,     "to":10000, "basis":0,    "perc":0.09},
    {"cat":2, "from":10000, "to":20000, "basis":900,  "perc":0.14},
    {"cat":3, "from":20000, "to":30000, "basis":2300, "perc":0.19},
    {"cat":4, "from":30000, "to":60000, "basis":4200, "perc":0.23},
    {"cat":5, "from":60000, "to":90000, "basis":11100,"perc":0.27},
    {"cat":6, "from":90000, "to":120000,"basis":19200,"perc":0.31},
    {"cat":7, "from":120000,"to":999999,"basis":28500,"perc":0.35}
  ],
  tabla = {
    SB: {val:0,concepto:"Sueldo Bruto"},
    MNI:{val:0,concepto:"Minimo no Imponible"},
    GBA:{val:0,concepto:"Ganancia Bruta Anual"},
    GNA:{val:0,concepto:"Ganancia Neta Anual"},
    GNI:{val:0,concepto:"Ganancia no Imponible"},
    GI: {val:0,concepto:"Ganancia Imponible"},
    IGA:{val:0,concepto:"Imp. a las ganancias Anual"},
    IGM:{val:0,concepto:"Imp. a las ganancias Mensual"},
    NF: {val:0,concepto:"Neto Final"},
  };

//-----------------------------------------------
function calcularValoresIniciales(){
  with(tabla){
    SB.val = Number($('#SB').val());
    GBA.val = (SB.val*13);
    GNA.val = GBA.val*(1-cargas);
    MNI.val = ($('#soltero').is(':checked'))?18880:25000;
    GNI.val = MNI.val*13;
    GI.val = GNA.val-GNI.val;
  }
}

function obtenerCategoria(){ //poner un foreach
  for(var i=0; i<categorias.length; i++){
    if(tabla.GI.val<categorias[i].to){
      categoria=categorias[i];
      break;
    }
  }
}

function calcularGanancias(){
  tabla.IGA.val = (categoria.basis+(categoria.perc*(tabla.GI.val-categoria.from)));
  tabla.IGM.val = (tabla.IGA.val/13);
  tabla.NF.val = (tabla.SB.val*(1-cargas)-tabla.IGM.val)
}

function imprimirTabla(){
  $('.valores').html("");
  $.each(tabla,function(index, elem){
    elem.val=elem.val.toFixed(2);
    $('.valores').append("<tr><th>"+elem.concepto+"</th><td>$"+elem.val+"</td></tr>")
  })
}
function mostrarDesgloce(){
  with(tabla){
    $('.bruto').text("$ "+SB.val)
    $('.carga').text("$ "+(SB.val*cargas).toFixed(2))
    $('.ganancias').text("$ "+IGM.val)
    $('.neto').text("$ "+NF.val)
  }
}
//-----------------------------------------------
$(document).ready(function(){
  $('#calcular').on('click',function(){
    calcularValoresIniciales();
    obtenerCategoria();
    if (tabla.GNA.val>tabla.GNI.val){
      calcularGanancias();
    }else{
      with(tabla){
        GI.val=0;
        IGA.val=0;
        IGM.val=0;
        NF.val=tabla.SB.val*(1-cargas);
      }
    }
    imprimirTabla();
    mostrarDesgloce();
  });
});
//agregar tooltips explicando c/concepto
//generar acordeon en conceptos agrupados
//fixear el ancho de la tabla
