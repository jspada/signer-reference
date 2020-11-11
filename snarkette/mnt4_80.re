open Fields;
module N = Nat;

module Fq =
  Make_fp(
    N,
    {
      let order =
        N.of_string(
          "475922286169261325753349249653048451545124879242694725395555128576210262817955800483758081",
        );
    },
  );

let non_residue = Fq.of_int(17);

module Fq2 = {
  module Params = {
    let non_residue = non_residue;
  };

  include Make_fp2(Fq, Params);
};

module Fq4 = {
  module Params = {
    let frobenius_coeffs_c1 = [|
      Fq.of_string("1"),
      Fq.of_string(
        "7684163245453501615621351552473337069301082060976805004625011694147890954040864167002308",
      ),
      Fq.of_string(
        "475922286169261325753349249653048451545124879242694725395555128576210262817955800483758080",
      ),
      Fq.of_string(
        "468238122923807824137727898100575114475823797181717920390930116882062371863914936316755773",
      ),
    |];

    let non_residue = Fq.(zero, one);
  };

  include Fields.Make_fp2(Fq2, Params);
};

module G1 = {
  module Params = {
    let a = Fq.of_string("2");

    let b =
      Fq.of_string(
        "423894536526684178289416011533888240029318103673896002803341544124054745019340795360841685",
      );
  };

  include Elliptic_curve.Make(N, Fq, Params);

  let one =
    of_affine((
      Fq.of_string(
        "336685752883082228109289846353937104185698209371404178342968838739115829740084426881123453",
      ),
      Fq.of_string(
        "402596290139780989709332707716568920777622032073762749862342374583908837063963736098549800",
      ),
    ));
};

module G2 = {
  module Params = {
    let a = Fq.(G1.Params.a * non_residue, zero);

    let b = Fq.(zero, G1.Params.b * non_residue);
  };

  include Elliptic_curve.Make(N, Fq2, Params);

  let one =
    of_affine(
      Fq.(
        (
          of_string(
            "438374926219350099854919100077809681842783509163790991847867546339851681564223481322252708",
          ),
          of_string(
            "37620953615500480110935514360923278605464476459712393277679280819942849043649216370485641",
          ),
        ),
        (
          of_string(
            "37437409008528968268352521034936931842973546441370663118543015118291998305624025037512482",
          ),
          of_string(
            "424621479598893882672393190337420680597584695892317197646113820787463109735345923009077489",
          ),
        ),
      ),
    );
};

module Pairing_info = {
  let twist = Fq.(zero, one);

  let loop_count =
    N.of_string("689871209842287392837045615510547309923794944");

  let is_loop_count_neg = false;

  let final_exponent =
    N.of_string(
      "107797360357109903430794490309592072278927783803031854357910908121903439838772861497177116410825586743089760869945394610511917274977971559062689561855016270594656570874331111995170645233717143416875749097203441437192367065467706065411650403684877366879441766585988546560",
    );

  let final_exponent_last_chunk_abs_of_w0 =
    N.of_string("689871209842287392837045615510547309923794945");

  let final_exponent_last_chunk_is_w0_neg = false;

  let final_exponent_last_chunk_w1 = N.of_string("1");
};
