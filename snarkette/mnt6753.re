open Core_kernel;
open Fields;
module N = Nat;

module Fq =
  Make_fp(
    N,
    {
      let order =
        N.of_string(
          "41898490967918953402344214791240637128170709919953949071783502921025352812571106773058893763790338921418070971888458477323173057491593855069696241854796396165721416325350064441470418137846398469611935719059908164220784476160001",
        );
    },
  );

let non_residue = Fq.of_int(11);

module Fq3 = {
  module Params = {
    let non_residue = non_residue;

    let frobenius_coeffs_c1 = [|
      Fq.of_string("1"),
      Fq.of_string(
        "24129022407817241407134263419936114379815707076943508280977368156625538709102831814843582780138963119807143081677569721953561801075623741378629346409604471234573396989178424163772589090105392407118197799904755622897541183052132",
      ),
      Fq.of_string(
        "17769468560101711995209951371304522748355002843010440790806134764399814103468274958215310983651375801610927890210888755369611256415970113691066895445191924931148019336171640277697829047741006062493737919155152541323243293107868",
      ),
    |];

    let frobenius_coeffs_c2 = [|
      Fq.of_string("1"),
      Fq.of_string(
        "17769468560101711995209951371304522748355002843010440790806134764399814103468274958215310983651375801610927890210888755369611256415970113691066895445191924931148019336171640277697829047741006062493737919155152541323243293107868",
      ),
      Fq.of_string(
        "24129022407817241407134263419936114379815707076943508280977368156625538709102831814843582780138963119807143081677569721953561801075623741378629346409604471234573396989178424163772589090105392407118197799904755622897541183052132",
      ),
    |];
  };

  include Make_fp3(Fq, Params);
};

module Fq2 =
  Make_fp2(
    Fq,
    {
      let non_residue = non_residue;
    },
  );

module Fq6 = {
  module Params = {
    let non_residue = non_residue;

    let frobenius_coeffs_c1 =
      Array.map(
        ~f=Fq.of_string,
        [|
          "1",
          "24129022407817241407134263419936114379815707076943508280977368156625538709102831814843582780138963119807143081677569721953561801075623741378629346409604471234573396989178424163772589090105392407118197799904755622897541183052133",
          "24129022407817241407134263419936114379815707076943508280977368156625538709102831814843582780138963119807143081677569721953561801075623741378629346409604471234573396989178424163772589090105392407118197799904755622897541183052132",
          "41898490967918953402344214791240637128170709919953949071783502921025352812571106773058893763790338921418070971888458477323173057491593855069696241854796396165721416325350064441470418137846398469611935719059908164220784476160000",
          "17769468560101711995209951371304522748355002843010440790806134764399814103468274958215310983651375801610927890210888755369611256415970113691066895445191924931148019336171640277697829047741006062493737919155152541323243293107868",
          "17769468560101711995209951371304522748355002843010440790806134764399814103468274958215310983651375801610927890210888755369611256415970113691066895445191924931148019336171640277697829047741006062493737919155152541323243293107869",
        |],
      );
  };

  include Make_fp6(N, Fq, Fq2, Fq3, Params);
};

module G1 = {
  include Elliptic_curve.Make(
            N,
            Fq,
            {
              let a = Fq.of_string("11");

              let b =
                Fq.of_string(
                  "11625908999541321152027340224010374716841167701783584648338908235410859267060079819722747939267925389062611062156601938166010098747920378738927832658133625454260115409075816187555055859490253375704728027944315501122723426879114",
                );
            },
          );

  let one: t = (
    {
      x:
        Fq.of_string(
          "3458420969484235708806261200128850544017070333833944116801482064540723268149235477762870414664917360605949659630933184751526227993647030875167687492714052872195770088225183259051403087906158701786758441889742618916006546636728",
        ),
      y:
        Fq.of_string(
          "27460508402331965149626600224382137254502975979168371111640924721589127725376473514838234361114855175488242007431439074223827742813911899817930728112297763448010814764117701403540298764970469500339646563344680868495474127850569",
        ),
      z: Fq.one,
    }: t
  );
};

module G2 = {
  include Elliptic_curve.Make(
            N,
            Fq3,
            {
              let a: Fq3.t = ((Fq.zero, Fq.zero, G1.Coefficients.a): Fq3.t);

              let b: Fq3.t = (
                (Fq.(G1.Coefficients.b * Fq3.non_residue), Fq.zero, Fq.zero): Fq3.t
              );
            },
          );

  let one: t = (
    Fq.{
      z: Fq3.one,
      x: (
        of_string(
          "27250797394340459586637772414334383652934225310678303542554641987990991970766156209996739240400887081904395745019996048910447071686918567661896491214767494514394154061111870331668445455228882471000120574964265209669155206168252",
        ),
        of_string(
          "35762481056967998715733586393399457882827322353696313323665483142561285210083843314423554450886956650265947502285422529615273790981238406393402603210224104850580302463396274854098657541573494421834514772635884262388058080180368",
        ),
        of_string(
          "36955296703808958167583270646821654948157955258947892285629161090141878438357164213613114995903637211606408001037026832604054121847388692538440756596264746452765613740820430501353237866984394057660379098674983614861254438847846",
        ),
      ),
      y: (
        of_string(
          "2540920530670785421282147216459500299597350984927286541981768941513322907384197363939300669100157141915897390694710534916701460991329498878429407641200901974650893207493883271892985923686300670742888673128384350189165542294615",
        ),
        of_string(
          "7768974215205248225654340523113146529854477025417883273460270519532499370133542215655437897583245920162220909271982265882784840026754554720358946490360213245668334549692889019612343620295335698052097726325099648573158597797497",
        ),
        of_string(
          "21014872727619291834131369222699267167761185012487859171850226473555446863681002782100371394603357586906967186931035615146288030444598977758226767063525819170917389755555854704165900869058188909090444447822088242504281789869689",
        ),
      ),
    }: t
  );
};

module Pairing_info = {
  let twist: Fq3.t = (Fq.(zero, one, zero): Fq3.t);

  let loop_count =
    N.of_string(
      "204691208819330962009469868104636132783269696790011977400223898462431810102935615891307667367766898917669754470400",
    );

  let is_loop_count_neg = false;

  let final_exponent =
    N.of_string(
      "129119521415595396014710306456032421075529786121916339618043051454538645105373777417137765707049510513015090026587997279208509759539952171373399816556184658054246934445122434683712249758515142075912382855071692226902812699306965286452865875620478620415339135536651578138124630852841411245063114044076427626521354349718502952988285309849333541213630352110932043828698936614474460281448819530109126473106492442797180252857193080048552501189491359213783058841481431978392771722128135286229420891567559544903231970966039315305865230923024300814788334307759652908820805819427293129932717325550045066338621261382334584633469485279042507653112873505613662346162595624798718660978835342384244182483671072189980911818690903244207181753883232560300278713216908336381030175242331281836803196022816489406715804002685525498662502919760346302653911463614694097216541218340832160715975576449518733830908486041613391828183354500089193133793376316346927602330584336604894214847791219714282509301093232896394808735738348953422584365914239193758384912179069975047674736700432948221178135004609440079320720726286913134205559121306917942266019404840960000",
    );

  let final_exponent_last_chunk_abs_of_w0 =
    N.of_string(
      "204691208819330962009469868104636132783269696790011977400223898462431810102935615891307667367766898917669754470400",
    );

  let final_exponent_last_chunk_is_w0_neg = false;

  let final_exponent_last_chunk_w1 = N.of_string("1");
};

module Pairing = Pairing.Make(Fq, Fq3, Fq6, G1, G2, Pairing_info);

module Inputs = {
  module N = N;
  module G1 = G1;
  module G2 = G2;
  module Fq = Fq;
  module Fqe = Fq3;
  module Fq_target = Fq6;
  module Pairing = Pairing;
};

module Groth_maller = Groth_maller.Make(Inputs);
module Groth16 = Groth16.Make(Inputs);

module Make_bowe_gabizon =
       (
         M: {
           let hash:
             (
               ~message: array(Fq.t)=?,
               ~a: G1.t,
               ~b: G2.t,
               ~c: G1.t,
               ~delta_prime: G2.t
             ) =>
             G1.t;
         },
       ) =>
  Bowe_gabizon.Make({
    include Inputs;

    let hash = M.hash;
  });
