open Core_kernel;

/*
   Define
   p = 2^254 + 4707489545178046908921067385359695873 = 28948022309329048855892746252171976963322203655955319056773317069363642105857
   q = 2^254 + 4707489544292117082687961190295928833 = 28948022309329048855892746252171976963322203655954433126947083963168578338817

   "Curve" is a module implementing the elliptic curve with defining equation

    y^2 = x^3 + 5

   defined over the field Fq of order q. This curve has scalar-field the field Fp of order p.
 */

module Field = Base_field;

let initial_state = {
  let prefix_to_field = (s: string) => {
    let bits_per_character = 8;
    assert(bits_per_character * String.length(s) < Field.length_in_bits);
    Field.project_bits(Fold_lib.Fold.(to_list(string_bits((s :> string)))));
  };

  let s = "CodaSignature*******";
  assert(String.length(s) == 20);
  Poseidon.hash_state(
    ~init=[|Field.zero, Field.zero, Field.zero|],
    [|prefix_to_field(s)|],
  );
};

module Curve = {
  include Snarkette.Tweedle.Dee;

  let scale = (p: t, x: Scalar_field.t) =>
    scale(p, Scalar_field.to_bigint(x));
};

module Poseidon = Poseidon;

module Private_key = {
  /* An Fp element */
  type t = Scalar_field.t;
};

module Signature = {
  [@deriving yojson]
  type t = (Field.t, Scalar_field.t);
};

module Message = {
  /* A message is a "random oracle input", which is a struct containing
        - an array of field elements
        - an array of bitstrings.

        They are kept separate because we use an arithmetic hash function (Poseidon) which
        natively accepts arrays of field elements as inputs.

        When we actually need to hash such a value, we "pack" all the bitstrings
        into as few field elements as we can and then feed that to the hash function
        along with the array of field elements.
     */
  type t = Random_oracle_input.t(Field.t, bool);

  /* The implementation of this function is not important and
     the verification procedure does not depend on it. This is
     here to avoid having to sample a random nonce when signing.

     Instead of sampling a random nonce, we hash together the
     private key, public key, and the message using a hash function
     which is modelled as a random oracle. Here we use blake2 but
     any hash function could be used. */
  let derive = (t, ~private_key, ~public_key) => {
    let input = {
      let (x, y) = Curve.to_affine_exn(public_key);
      /* "append" works by concatenating the field element arrays
         and the bitstring arrays in the two "inputs" */
      Random_oracle_input.append(
        t,
        {
          field_elements: [|x, y|],
          bitstrings: [|Scalar_field.to_bits(private_key)|],
        },
      );
    };

    Random_oracle_input.to_bits(~unpack=Field.to_bits, input)
    |> Array.of_list
    |> Blake2.bits_to_string
    |> Blake2.digest_string
    |> Blake2.to_raw_string
    |> Blake2.string_to_bits
    |> Array.to_list
    |> Fn.flip(List.take, Scalar_field.length_in_bits - 1)
    |> Scalar_field.project_bits;
  };

  /* Hash the message together with the public key and [r], and use the output as the Schnorr challenge. */
  let hash = (t, ~public_key, ~r) => {
    let input = {
      let (px, py) = Curve.to_affine_exn(public_key);
      Random_oracle_input.append(
        t,
        {field_elements: [|px, py, r|], bitstrings: [||]},
      );
    };

    /* "init" is the initial state vector of 3 Fq elements.
          it is equal to the hash state after consuming the string

          "CodaSignature*******"

          where the string is interpreted as a field element by converting
          it to bits (internal byte order being little endian) and then interpreting
          the resulting 160 bits as a little-endian representation of a field element.
       */
    Poseidon.digest(~init=initial_state, Random_oracle_input.pack(input))
    /* Poseidon returns an Fq element. We convert it to an Fp element by converting the Fq element to a little endian bit
       string and then converting that little endian bit string to an Fp element. */
    |> Field.to_bits
    |> Scalar_field.project_bits;
  };
};

let is_even = (t: Field.t) => !Field.Nat.test_bit(Field.to_bigint(t), 0);

let sign = (d_prime: Private_key.t, m) => {
  let public_key = Curve.scale(Curve.one, d_prime);
  let d = d_prime;
  let k_prime = Message.derive(m, ~public_key, ~private_key=d);
  assert(!Scalar_field.(equal(k_prime, zero)));
  let (r, ry) = Curve.(to_affine_exn(scale(Curve.one, k_prime)));
  let k =
    if (is_even(ry)) {
      k_prime;
    } else {
      Scalar_field.negate(k_prime);
    };
  let e = Message.hash(m, ~public_key, ~r);
  let s = Scalar_field.(k + e * d);
  (r, s);
};

let verify = ((r, s): Signature.t, pk: Public_key.t, m: Message.t) => {
  let e = Message.hash(~public_key=pk, ~r, m);
  let r_pt = Curve.(scale(one, s) + negate(scale(pk, e)));
  switch (Curve.to_affine_exn(r_pt)) {
  | (rx, ry) => is_even(ry) && Field.equal(rx, r)
  | exception _ => false
  };
};

let main = (privkey, json_path) => {
  let payload =
    switch (Transaction.Payload.of_yojson(Yojson.Safe.from_file(json_path))) {
    | Ok(x) => x
    | Error(e) => failwithf("Could not read transaction paylod: %s", e, ())
    };

  let privkey =
    switch (privkey) {
    | None => Scalar_field.random()
    | Some(x) => Scalar_field.of_string(x)
    };

  let message = Transaction.Payload.to_input(payload);
  let pubkey = Curve.scale(Curve.one, privkey);
  let signature = sign(privkey, message);
  assert(verify(signature, pubkey, message));
};

module Test = {
  /* A test */
  let () = {
    let message =
      Random_oracle_input.field_elements([|
        Field.of_int(1),
        Field.of_int(2),
        Field.of_int(345),
      |]);

    let privkey = Scalar_field.random();
    let pubkey = Curve.scale(Curve.one, privkey);
    let signature = sign(privkey, message);
    assert(verify(signature, pubkey, message));
    assert(
      !
        verify(
          signature,
          pubkey,
          Random_oracle_input.field_elements([|Field.of_int(222)|]),
        ),
    );
  };

  let private_key =
    Scalar_field.of_string(
      "12582171893661899879526366534412228500822795919500703333934216608017454457854",
    );

  let transactions = {
    module T = {
      [@deriving yojson]
      type t = list(Transaction.Payload.t);
    };
    {json|[{"common":{"fee":"0.000000003","fee_token":"1","fee_payer_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","nonce":"200","valid_until":"10000","memo":"E4Yq8cQXC1m9eCYL8mYtmfqfJ5cVdhZawrPQ6ahoAay1NDYfTi44K"},"body":["Payment",{"source_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","receiver_pk":"B62qnekV6LVbEttV7j3cxJmjSbxDWuXa5h3KeVEXHPGKTzthQaBufrY","token_id":"1","amount":"0.000000042"}]},{"common":{"fee":"0.000000015","fee_token":"1","fee_payer_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","nonce":"212","valid_until":"305","memo":"E4Yxub7Sz9ArM75kQ4mpHCiWiZtaoFYM9AiC8YevcfnHPNSRt31Ea"},"body":["Payment",{"source_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","receiver_pk":"B62qnekV6LVbEttV7j3cxJmjSbxDWuXa5h3KeVEXHPGKTzthQaBufrY","token_id":"1","amount":"0.000002048"}]},{"common":{"fee":"0.000002001","fee_token":"1","fee_payer_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","nonce":"3050","valid_until":"9000","memo":"E4YziQuAG1u7X6CFjm7QJCtgjXZJFU1eKbC1Bsjmkfc8a8LzL1yFa"},"body":["Payment",{"source_pk":"B62qkef7po74VEvJYcLYsdZ83FuKidgNZ8Xiaitzo8gKJXaxLwxgG7T","receiver_pk":"B62qnekV6LVbEttV7j3cxJmjSbxDWuXa5h3KeVEXHPGKTzthQaBufrY","token_id":"1","amount":"0.000000109"}]}]|json}
    |> Yojson.Safe.from_string
    |> T.of_yojson
    |> Result.map_error(~f=Error.of_string)
    |> Or_error.ok_exn;
  };

  let expected_signatures = [
    (
      Base_field.of_string(
        "26465527983186209303505416012999471216791929294547202544418628218684401247482",
      ),
      Scalar_field.of_string(
        "24314622607568634019238925194003982035713282123495098742563304525462390474675",
      ),
    ),
    (
      Base_field.of_string(
        "17904579124856301623430254602493845052225066133783759830933851350565024476791",
      ),
      Scalar_field.of_string(
        "23236310451221515151306332214866075609530591126304104292719087769412261542996",
      ),
    ),
    (
      Base_field.of_string(
        "3773739279495617006859274664335336703544299779357622700969076708386497770638",
      ),
      Scalar_field.of_string(
        "16564414243193951049308136709360383379865035843900659892396017769733949676574",
      ),
    ),
  ];

  let () = {
    let signatures =
      List.map(
        ~f=Fn.compose(sign(private_key), Transaction.Payload.to_input),
        transactions,
      );

    List.iter2_exn(signatures, expected_signatures, ~f=(x, y) =>
      [%test_eq: (Base_field.t, Scalar_field.t)](x, y)
    );
  };
};

let () =
  Command.basic(
    ~summary="Signing utility",
    {
      open Command.Param;
      open Command.Let_syntax;
      let%map privkey =
        flag(
          "privkey",
          ~doc="Private key as a base 10 string",
          optional(string),
        )
      and json_path = anon("FILE" %: string);
      () => main(privkey, json_path);
    },
  )
  |> Core.Command.run;

/* For signing transactions, we need to specify coda's transaction format and
      how transactions are converted into "random oracle input" structs

      A coda transaction has the structure

      { payload:
       { common:
           { fee: Fee.t
           , fee_token: Token_id.t
           , fee_payer_pk: Public_key.Compressed.t
           , nonce: Nonce.t
           , valid_until: Global_slot.t
           , memo: char[34] }
       , body:
           { tag: Tag.t
           , source_pk: Public_key.Compressed.t
           , receiver_pk: Public_key.Compressed.t
           , token_id: Token_id.t
           , amount: Amount.t }
       }
      , signer: Public_key.Compressed.t
      , signature: Signature.t
      }

      - Amount.t is a uint64
      - Fee.t is a uint64
      - Global_slot.t is a uint32
      - Token_id.t is a uint64
      - the tag is the three bits 000 for a payment and 001 for a stake delegation

      To sign this, we convert the "payload" field to an "random oracle input", i.e., an
      array of field elements and an array of bitstrings.

      The "inputs" struct this gets reduced to looks like
      { field_elements=[|
         fee_payer_pk.x,
         source_pk.x, receiver_pk.x
        |]
      , bitstrings= [|
         fee, fee_token, fee_payer_pk.is_odd, nonce, valid_until, memo,
         tag, [source_pk.is_odd], [receiver_pk.is_odd], token_id, amount
        |]
      }
   */
