var cargas=0.17, texto,
  categorias = [
    {from:0,     to:10000, basis:0,    perc:0.09},
    {from:10000, to:20000, basis:900,  perc:0.14},
    {from:20000, to:30000, basis:2300, perc:0.19},
    {from:30000, to:60000, basis:4200, perc:0.23},
    {from:60000, to:90000, basis:11100,perc:0.27},
    {from:90000, to:120000,basis:19200,perc:0.31},
    {from:120000,to:999999,basis:28500,perc:0.35}
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
    tabla.SB.val = Number($('#SB').val());
    tabla.GBA.val = (tabla.SB.val*13);
    tabla.GNA.val = tabla.GBA.val*(1-cargas);
    tabla.MNI.val = ($('#soltero').is(':checked'))?18880:25000;
    tabla.GNI.val = tabla.MNI.val*13;
    tabla.GI.val = tabla.GNA.val-tabla.GNI.val;
}
function calcularGanancias(){
  var cat=null;
  for(var i=0;i<categorias.length && !cat;){
    (tabla.GI.val<categorias[i].to)?cat=i:i++;
  }
  tabla.IGA.val = (categorias[i].basis+(categorias[i].perc*(tabla.GI.val-categorias[i].from)));
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
    $('.bruto').text("$ "+tabla.SB.val)
    $('.carga').text("$ "+(tabla.SB.val*cargas).toFixed(2))
    $('.ganancias').text("$ "+tabla.IGM.val)
    $('.neto').text("$ "+tabla.NF.val)
}
//-----------------------------------------------
$(document).ready(function(){
  $('#calcular').on('click',function(){
    calcularValoresIniciales();
    if (tabla.GNA.val>tabla.GNI.val){
      calcularGanancias();
    }else{
        tabla.GI.val=0;
        tabla.IGA.val=0;
        tabla.IGM.val=0;
        tabla.NF.val=tabla.SB.val*(1-cargas);
    }
    imprimirTabla();
    mostrarDesgloce();
  });
});
